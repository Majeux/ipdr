#include "frames.h"
#include "frame.h"
#include "logger.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
  using std::optional;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3ext::solver::Witness;

  Frames::Frames(Context c, IModel& m, Logger& l)
      : ctx(c),
        model(m),
        log(l),
        FI_solver(ctx, model, m.get_initial(), m.get_transition(),
            m.get_constraint()),
        delta_solver(
            ctx, model, m.property, m.get_transition(), m.get_constraint()),
        init_solver(ctx)
  {
    // the initial states always remain the same
    init_solver.reset();
    init_solver.add(model.get_initial());

    frames.emplace_back(0);
    act.push_back(ctx().bool_const("__actI__")); // unused

    extend();
    assert(frontier() == 0);

    MYLOG_DEBUG(log, "FI_solver after init {}", FI_solver.as_str("", false));
    MYLOG_DEBUG(log, "solver after init {}", delta_solver.as_str("", false));
  }

  void Frames::reset()
  {
    assert(frames.size() == act.size());

    frames.clear();
    act.clear();

    frames.emplace_back(0);
    act.push_back(ctx().bool_const("__actI__")); // unused

    extend();
    assert(frontier() == 0);

    FI_solver.remake(
        model.get_initial(), model.get_transition(), model.get_constraint());
    delta_solver.remake(
        model.property, model.get_transition(), model.get_constraint());
  }

  optional<size_t> Frames::reuse()
  {
    assert(frames.size() > 0);
    assert(model.diff == IModel::Diff_t::constrained);

    delta_solver.reconstrain_clear(model.get_constraint());

    // repopulate
    for (size_t i{ 1 }; i < frames.size(); i++)
      delta_solver.block(frames[i].get_blocked(), act.at(i));

    // with fewer transitions, new cubes may be propagated
    MYLOG_INFO(log, "Redoing last propagation: {}", frontier() - 1);

    model.diff = IModel::Diff_t::none;
    return propagate(frontier() - 1);
  }

  void Frames::reset_to_F1()
  {
    assert(frames.size() > 0);
    assert(model.diff == IModel::Diff_t::relaxed);
    MYLOG_INFO(log, "Reset frames to F1");

    // reconstrain solver and reset it to "no blocked"
    delta_solver.reconstrain_clear(model.get_constraint());
    z3ext::CubeSet old = get_blocked_in(1); // store all cubes in F_1
    clear_until(0);                         // reset sequence to { F_0 }
    extend();                               // reinstate level 1

    unsigned count = 0;
    for (vector<expr> const& cube : old)
    {
      if (SAT(0, z3ext::convert(cube)))
      {
#warning todo/future work regeneralize cti from this cube (must be possible or counter)
        // TODO regeneralize cti from this cube (must be possible or counter)
        // else it will be reconsidered next iteration
      }
      else
      {
        count++;
        remove_state(cube, 1);
      }
    }
    IF_STATS({
      log.stats.relax_copied_cubes_perc = (double)count / old.size() * 100.0;
    });
    MYLOG_DEBUG(log, "{} cubes carried over, out of {}", count, old.size());
    MYLOG_DEBUG(
        log, "Repopulated solver: \n{}", delta_solver.as_str("", false));

    model.diff = IModel::Diff_t::none;
  }

  // frame interface
  //
  // removes frames until the frontier is the given argument
  void Frames::clear_until(size_t frontier_index)
  {
    assert(frames.size() == act.size());

    // pop until given index is the highest
    while (frontier() > frontier_index)
    {
      frames.pop_back();
      act.pop_back();
    }
  }

  void Frames::extend()
  {
    assert(frames.size() > 0);
    std::string acti = fmt::format("__act{}__", frames.size());
    act.push_back(ctx().bool_const(acti.c_str()));
    frames.emplace_back(frames.size());
  }

#warning TODO: refresh solver based on % or no. subsumed ??
  void Frames::repopulate_solvers()
  {
    delta_solver.reset();
    for (size_t i = 1; i < frames.size(); i++)
      delta_solver.block(frames[i].get_blocked(), act.at(i));
  }

  z3ext::CubeSet Frames::get_blocked_in(size_t i) const
  {
    assert(i < frames.size());
    z3ext::CubeSet blocked;

    // all cubes across in all delta-levels belong to F_1
    for (; i < frames.size(); i++)
    {
      // TODO non-const getter allows std::move
      z3ext::CubeSet const& Fi = frames[i].get_blocked();
      blocked.insert(Fi.begin(), Fi.end());
    }

    return blocked;
  }

  bool Frames::remove_state(std::vector<expr> const& cube, size_t level)
  {
    assert(level < frames.size());
    // level = std::min(level, frames.size() - 1);
    MYLOG_DEBUG(log, "removing cube from level [1..{}]: [{}]", level,
        z3ext::join_ev(cube));

    log.indent++;
    bool result = delta_remove_state(cube, level);
    log.indent--;
    return result;
  }

  bool Frames::delta_remove_state(std::vector<expr> const& cube, size_t level)
  {
    for (unsigned i = 1; i <= level; i++)
    {
      // remove all blocked cubes that are equal or weaker than cube
      unsigned n_removed = frames.at(i).remove_subsumed(cube, i < level);
      (void)n_removed;
      IF_STATS(log.stats.subsumed_cubes.add(level, n_removed));
    }

    assert(level > 0 && level < frames.size());
#warning TODO consider storing vectors of expr instead of expr_vectors
    if (frames[level].block(cube))
    {
      delta_solver.block(cube, act.at(level));
      MYLOG_DEBUG(log, "blocked in {}", level);
      return true;
    }
    else
    {
      MYLOG_DEBUG(log, "was already blocked in {}", level);
    }

    return false;
  }

  std::optional<size_t> Frames::propagate() { return propagate(frontier()); }
  std::optional<size_t> Frames::propagate(size_t k)
  {
    assert(k <= frontier());
    MYLOG_INFO(log, "propagate levels {} - {}", 1, k);
    log_solver(true);
    log.indent++;
    bool repeat = (k < frontier());

    for (size_t i = 1; i <= k; i++)
      push_forward_delta(i, repeat);

    for (size_t i = 1; i <= k; i++)
      if (frames.at(i).empty())
      {
        MYLOG_INFO(log, "F[{}] \\ F[{}] == 0", i, i + 1);
        return i;
      }

    repopulate_solvers();
    log.indent--;
    log_solver(true);

    return {};
  }

  void Frames::push_forward_delta(size_t level, bool repeat)
  {
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    unsigned count         = 0;
    z3ext::CubeSet blocked = frames.at(level).get_blocked();
    for (std::vector<expr> const& cube : blocked)
    {
      if (!trans_source(level, cube))
      {
        if (remove_state(cube, level + 1))
          if (repeat)
            count++;
      }
    }
    if (repeat)
      MYLOG_TRACE(log, "{} blocked in repeat", count);

    std::chrono::duration<double> dt(steady_clock::now() - start);
    IF_STATS(log.stats.propagation_level.add(level, dt.count()));
  }

  //
  // end frame interface

  // queries
  //
  // verifies if !cube is inductive relative to F_[frame]
  // query: Fi & !cube & T /=> !cube'
  bool Frames::inductive(std::vector<z3::expr> const& cube, size_t frame)
  {
    MYLOG_TRACE(log, "check relative inductiveness, frame{}", frame);

    z3::expr clause =
        z3::mk_or(z3ext::negate(cube)); // negate cube via demorgan
    z3::expr_vector assumptions = model.vars.p(cube); // cube in next state
    assumptions.push_back(clause);

    if (SAT(frame, std::move(assumptions)))
      return false; // there is a transition from !s to s'
    return true;
  }

  std::optional<vector<expr>> Frames::counter_to_inductiveness(
      std::vector<z3::expr> const& cube, size_t frame)
  {
    MYLOG_TRACE(log, "get counter relative inductiveness, frame{}", frame);

    if (!inductive(cube, frame))
      return get_solver(frame).std_witness_current();

    return {};
  }

  // if primed: cube is already in next state, else first convert it
  bool Frames::trans_source(
      size_t frame, vector<expr> const& dest_cube, bool primed)
  {
    MYLOG_TRACE(log, "check transition source, frame{}", frame);
    if (!primed) // cube is in current, bring to next
      return SAT(frame, model.vars.p(dest_cube));

    return SAT(frame,
        z3ext::convert(dest_cube)); // there is a transition from Fi to s'
  }

  std::optional<Witness> Frames::get_trans_source(
      size_t frame, vector<expr> const& dest_cube, bool primed)
  {
    MYLOG_TRACE(log, "get transition source, frame{}", frame);

    if (!primed) // cube is in current, bring to next
      if (!SAT(frame, model.vars.p(dest_cube)))
        return {};

    if (!SAT(frame, z3ext::convert(dest_cube)))
      return {};

    // else there exists a source -T-> dest'
    vector<expr> curr = get_solver(frame).std_witness_current();
    vector<expr> next =
        get_solver(frame).filter_witness_vector(get_solver(frame).get_model(),
            [this](const expr l) { return model.vars.lit_is_p(l); });
    return Witness(curr, next);
  }

  //
  // end queries

  // SAT interface
  //
  bool Frames::SAT(size_t frame, z3::expr_vector const& assumptions)
  {
    return SAT(frame, z3ext::copy(assumptions));
  }

  // the expr_vector assumptions are modified by acts,
  // and should be considered unusable afterwards
  bool Frames::SAT(size_t frame, z3::expr_vector&& assumptions)
  {
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    MYLOG_TRACE(log, "SAT-query: F_0..F_{} & T", frame);

    if (frame > 0)
    {
      assert(frames.size() == act.size());
      for (unsigned i = frame; i < act.size(); i++)
        assumptions.push_back(act[i]);
    }

    log.indent++;
    MYLOG_TRACE(log, "assumptions: [ {} ]", z3ext::join_ev(assumptions, false));

    bool result = get_solver(frame).SAT(assumptions);
    std::chrono::duration<double> diff(steady_clock::now() - start);
    IF_STATS(log.stats.solver_calls.add(frontier(), diff.count()));

    log.indent--;
    MYLOG_TRACE(log, "result = {}", result == z3::sat ? "sat" : "unsat");

    return result;
  }

  const z3::model Frames::get_model(size_t frame) const
  {
    (void)frame;

    assert(frame > 0);
    assert(frame < frames.size());

    return delta_solver.get_model();
  }

  //
  // end SAT interface

  // getters
  //
  // the index 'k' to the second to last frame
  size_t Frames::frontier() const
  {
    assert(frames.size() > 1); // 0 is the minimal frontier (series F_0, F_1)
    return frames.size() - 2;
  }

  Solver& Frames::get_solver(size_t frame)
  {
    assert(frame < frames.size());

    if (frame == 0)
      return FI_solver;

    return delta_solver;
  }

  const Solver& Frames::get_solver(size_t frame) const
  {
    assert(frame < frames.size());

    if (frame == 0)
      return FI_solver;

    return delta_solver;
  }

  const Frame& Frames::operator[](size_t i)
  {
    assert(i > 0);
    assert(i < frames.size());
    return frames[i];
  }

  //
  // end getters

  void Frames::log_blocked() const
  {
    MYLOG_DEBUG(log, SEP3);
    MYLOG_DEBUG(log, blocked_str());
    MYLOG_DEBUG(log, SEP3);
  }

  void Frames::log_solver(bool only_clauses) const
  {
    (void)only_clauses;

    MYLOG_DEBUG(log, SEP3);
    log_blocked();
    MYLOG_DEBUG(log, FI_solver.as_str("", only_clauses));
    MYLOG_DEBUG(log, delta_solver.as_str("", only_clauses));
    MYLOG_DEBUG(log, SEP3);
  }

  std::string Frames::blocked_str() const
  {
    std::string str;
    for (auto& f : frames)
    {
      str += f.blocked_str();
      str += '\n';
    }
    return str;
  }
  std::string Frames::solver_str(bool only_clauses) const
  {
    return delta_solver.as_str("", only_clauses);
  }

} // namespace pdr
