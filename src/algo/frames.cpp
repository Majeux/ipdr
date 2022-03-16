#include "frames.h"
#include "_logging.h"
#include "frame.h"
#include "output.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
  Frames::Frames(context& c, Logger& l)
      : ctx(c), logger(l), frame_base(ctx()), init_solver(ctx())
  {
    const Model& m = ctx.const_model();
    init_solver.add(m.get_initial());
    frame_base = m.property.currents(); // all frames are initialized to P

    const z3::expr_vector& t      = m.get_transition();
    const z3::expr_vector& constr = m.get_cardinality();
    if (ctx.delta)
      delta_solver = std::make_unique<Solver>(ctx, frame_base, t, constr);

    init_frame_I();
  }

  void Frames::init_frame_I()
  {
    const Model& m = ctx.const_model();
    const z3::expr_vector& t      = m.get_transition();
    const z3::expr_vector& constr = m.get_cardinality();

    if (ctx.delta)
      act.push_back(ctx().bool_const("__actI__")); // unused

    // make F_0 = I transition solver
    auto frame_solver =
        std::make_unique<Solver>(ctx, m.get_initial(), t, constr);
    auto new_frame =
        std::make_unique<Frame>(0, std::move(frame_solver), logger);
    frames.push_back(std::move(new_frame));
  }

  // frame interface
  //
  void Frames::clear()
  {
    assert(frames.size() == act.size());
    while (frames.size() > 1) // F_0 always remains the same, can be kept
    {
      frames.pop_back();
      if (ctx.delta)
        act.pop_back();
    }
  }

  void Frames::extend()
  {
    assert(frames.size() > 0);
    if (ctx.delta)
    { // frame with its own solver
      std::string acti = fmt::format("__act{}__", frames.size());
      act.push_back(ctx().bool_const(acti.c_str()));
      frames.push_back(std::make_unique<Frame>(frames.size(), logger));
    }
    else
    { // frame with its own solver
      const Model& m = ctx.const_model();
      const z3::expr_vector& t      = m.get_transition();
      const z3::expr_vector& constr = m.get_cardinality();
      auto frame_solver = std::make_unique<Solver>(ctx, frame_base, t, constr);
      auto new_frame    = std::make_unique<Frame>(frames.size(),
                                               std::move(frame_solver), logger);
      frames.push_back(std::move(new_frame));
    }
  }

  // redefine constraint in the model
  // existing and future frames have a reference to this
  // then clean all solvers from old assertions and reblock all its cubes
  // TODO only really re-adjusts constraint for incremental/decremental
  void Frames::reset_constraint(Statistics& s, int x)
  {
    ctx.model().set_max_pebbles(x); 

    for (size_t i = 1; i < frames.size(); i++)
      frames[i]->set_stats(s);

    repopulate_solvers();
  }

  // reset solvers and repopulate with current blocked cubes
  void Frames::repopulate_solvers()
  {
    for (size_t i = 1; i < frames.size(); i++)
    {
      if (ctx.delta)
      {
        if (i == 1)
          delta_solver->reset();
        for (const z3::expr_vector& cube : frames[i]->get_blocked())
          delta_solver->block(cube, act.at(i));
      }
      else
        get_solver(i).reset(frames[i]->get_blocked());
    }
  }

  void Frames::increment_reset(Statistics& s, int x)
  {
    assert(ctx.type == Run::increment);
    assert(frames.size() > 0);
    assert(x > ctx.model().get_max_pebbles());
    ctx.model().set_max_pebbles(x); 

    for (size_t i = 1; i < frames.size(); i++)
      frames[i]->set_stats(s);

    CubeSet old = get_blocked(1); // store all cubes in F_1
    clear(); // reset sequence to { F_0 }
    extend(); //reinstate level 1

    for (const z3::expr_vector& cube : old)
    {
      if (SAT(0, cube))
      {
        // TODO regeneralize cti from this cube (must be possible or counter)
        // else it will be reconsidered next iteration
      }
      else
        remove_state(cube, 1);
    }
  }

  CubeSet Frames::get_blocked(size_t i) const
  {
    assert(i < frames.size());
    CubeSet blocked;

    if (ctx.delta)
    {
      delta_solver->reset();
      // all cubes in all delta-level belong to F_1
      for (; i < frames.size(); i++)
      {
        // TODO non-const getter allows std::move
        const CubeSet& Fi = frames[i]->get_blocked();
        blocked.insert(Fi.begin(), Fi.end());
      }
    }
    else
      blocked = frames[1]->get_blocked();

    return blocked;
  }

  bool Frames::remove_state(const z3::expr_vector& cube, size_t level)
  {
    level = std::min(level, frames.size() - 1);
    SPDLOG_LOGGER_TRACE(logger.spd_logger,
                        "{}| removing cube from level [1..{}]: [{}]",
                        logger.tab(), level, str::extend::join(cube));
    logger.indent++;

    bool result;
    if (ctx.delta) // a cube is only stored in the last frame it holds
      result = delta_remove_state(cube, level);
    else
      result = fat_remove_state(cube, level);
    logger.indent--;
    return result;
  }

  bool Frames::delta_remove_state(const z3::expr_vector& cube, size_t level)
  {
    for (unsigned i = 1; i <= level; i++)
    {
      // remove all blocked cubes that are equal or weaker than cube
      unsigned n_removed = frames.at(i)->remove_subsumed(cube);
      logger.stats.subsumed_cubes.add(level, n_removed);
    }

    assert(level > 0 && level < frames.size());
    if (frames[level]->block(cube))
    {
      delta_solver->block(cube, act.at(level));
      SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(),
                          level);
      return true;
    }
    return false;
  }

  bool Frames::fat_remove_state(const z3::expr_vector& cube, size_t level)
  {
    assert(level > 0 && level < frames.size());
    for (unsigned i = 1; i <= level; i++)
    {
      // remove all blocked cubes that are equal or weaker than cube
      unsigned n_removed = frames.at(i)->remove_subsumed(cube);
      logger.stats.subsumed_cubes.add(level, n_removed);

      if (frames[i]->block(cube))
      {
        frames[i]->block_in_solver(cube);
        SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}",
                            logger.tab(), i);
      }
      else
        return false;
    }
    return true;
  }

  int Frames::propagate(unsigned level, bool repeat)
  {
    assert(level == frontier() - 1); // k == |F|-1
    logger.out() << "propagate level " << level << std::endl;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| propagate frame {} to {}",
                        logger.tab(), 1, level);
    logger.indent++;

    for (unsigned i = 1; i <= level; i++)
    {
      if (ctx.delta)
        push_forward_delta(i, repeat);
      else if (push_forward_fat(i, repeat))
        return i;
    }

    if (ctx.delta)
      for (unsigned i = 1; i <= level; i++)
      {
        if (frames.at(i)->empty())
        {
          logger.out() << fmt::format("F[{}] \\ F[{}] == 0", i, i + 1)
                       << std::endl;
          return i;
        }
      }

    repopulate_solvers();
    logger.indent--;

    return -1;
  }

  void Frames::push_forward_delta(unsigned level, bool repeat)
  {
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    CubeSet blocked = frames.at(level)->get_blocked();
    for (const z3::expr_vector& cube : blocked)
    {
      if (!trans_source(level, cube))
      {
        if (remove_state(cube, level + 1))
          if (repeat)
            logger.out() << "new blocked in repeat" << std::endl;
      }
    }

    std::chrono::duration<double> dt(steady_clock::now() - start);
    logger.stats.propagation_level.add_timed(level, dt.count());
  }

  int Frames::push_forward_fat(unsigned level, bool repeat)
  {
    int rv = -1;
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    std::vector<z3::expr_vector> diff =
        frames.at(level)->diff(*frames.at(level + 1));
    for (const z3::expr_vector& cube : diff)
    {
      if (!trans_source(level, cube))
      {
        if (remove_state(cube, level + 1))
          if (repeat)
            logger.out() << "new blocked in repeat" << std::endl;
      }
    }

    if (diff.size() == 0 || frames.at(level)->equals(*frames.at(level + 1)))
    {
      logger.out() << fmt::format("F_{} \\ F_{} == 0", level, level + 1)
                   << std::endl;
      rv = level;
    }

    std::chrono::duration<double> dt(steady_clock::now() - start);
    logger.stats.propagation_level.add_timed(level, dt.count());
    return rv;
  }
  //
  // end frame interface

  // queries
  //
  bool Frames::inductive(const std::vector<z3::expr>& cube, size_t frame) const
  {
    return inductive(z3ext::convert(cube), frame);
  }

  // verifies if !cube is inductive relative to F_[frame]
  // query: Fi & !s & T /=> !s'
  bool Frames::inductive(const z3::expr_vector& cube, size_t frame) const
  {
    SPDLOG_LOGGER_TRACE(logger.spd_logger,
                        "{}| check relative inductiveness, frame {}",
                        logger.tab(), frame);
    z3::expr clause =
        z3::mk_or(z3ext::negate(cube)); // negate cube via demorgan
    z3::expr_vector assumptions =
        ctx.const_model().literals.p(cube); // cube in next state
    assumptions.push_back(clause);

    if (SAT(frame, std::move(assumptions)))
    { // there is a transition from !s to s'
      return false;
    }

    return true;
  }

  Witness Frames::counter_to_inductiveness(const std::vector<z3::expr>& cube,
                                           size_t frame) const
  {
    SPDLOG_LOGGER_TRACE(logger.spd_logger,
                        "{}| counter to relative inductiveness, frame {}",
                        logger.tab(), frame);
    if (!inductive(cube, frame))
      return std::make_unique<z3::model>(get_model(frame));

    return std::unique_ptr<z3::model>();
  }

  Witness Frames::counter_to_inductiveness(const z3::expr_vector& cube,
                                           size_t frame) const
  {
    if (!inductive(cube, frame))
      return std::make_unique<z3::model>(get_model(frame));

    return std::unique_ptr<z3::model>();
  }

  // if primed: cube is already in next state, else first convert it
  bool Frames::trans_source(size_t frame, const z3::expr_vector& dest_cube,
                            bool primed) const
  {
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| transition check, frame {}",
                        logger.tab(), frame);
    if (!primed) // cube is in current, bring to next
      return SAT(frame, ctx.const_model().literals.p(dest_cube));

    return SAT(frame, dest_cube); // there is a transition from Fi to s'
  }

  // std::unique_ptr<z3::expr_vector> Frames::get_trans_source(size_t frame,
  //                                          const z3::expr_vector& dest_cube,
  //                                          bool primed) const
  // {
  //   SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| transition query, frame {}",
  //                       logger.tab(), frame);
  //   Witness witness;
  //   if (!primed) // cube is in current, bring to next
  //     if (!SAT(frame, ctx.get_model().literals.p(dest_cube)))
  //       return std::unique_ptr<z3::expr_vector>();

  //   if (!SAT(frame, dest_cube))
  //     return std::unique_ptr<z3::expr_vector>();
  //   // else there exists a source -T-> dest'
  //   return get_solver(frame).witness_current();
  // }

  //
  // end queries

  // SAT interface
  //
  bool Frames::SAT(size_t frame, const z3::expr_vector& assumptions) const
  {
    return SAT(frame, z3ext::copy(assumptions));
  }

  // the expr_vector assumptions are modified by acts,
  // and should be considered usable afterwards
  bool Frames::SAT(size_t frame, z3::expr_vector&& assumptions) const
  {
    using std::chrono::steady_clock;
    auto start = steady_clock::now();

    Solver& solver = get_solver(frame);
    if (ctx.delta && frame > 0)
    {
      SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| Delta check", logger.tab());

      assert(frames.size() == act.size());
      for (unsigned i = frame; i <= frontier(); i++)
        assumptions.push_back(act.at(i));
    }
    else
      SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| Fat check", logger.tab());

    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| assumps: [ {} ]", logger.tab(),
                        z3ext::join_expr_vec(assumptions, false));
    logger.indent--;

    bool result = solver.SAT(assumptions);
    std::chrono::duration<double> diff(steady_clock::now() - start);
    logger.stats.solver_calls.add_timed(frontier(), diff.count());

    return result;
  }

  Witness Frames::SAT_model(size_t frame,
                            const z3::expr_vector& assumptions) const
  {
    if (SAT(frame, assumptions))
      return std::make_unique<z3::model>(get_model(frame));
    return std::unique_ptr<z3::model>();
  }

  Witness Frames::SAT_model(size_t frame, z3::expr_vector&& assumptions) const
  {
    if (SAT(frame, assumptions))
      return std::make_unique<z3::model>(get_model(frame));
    return std::unique_ptr<z3::model>();
  }

  const z3::model Frames::get_model(size_t frame) const
  {
    if (ctx.delta && frame > 0)
      return delta_solver->get_model();
    else
      return frames.at(frame)->get_solver().get_model();
  }

  //
  // end SAT interface

  // getters
  //
  unsigned Frames::frontier() const
  {
    assert(frames.size() > 0);
    return frames.size() - 1;
  }

  Solver& Frames::get_solver(size_t frame) const
  {
    assert(frame < frames.size());
    if (ctx.delta && frame > 0)
      return *delta_solver;
    return frames[frame]->get_solver();
  }

  const Solver& Frames::get_const_solver(size_t frame) const
  {
    assert(frame < frames.size());
    if (ctx.delta && frame > 0)
      return *delta_solver;
    return frames[frame]->get_const_solver();
  }

  const Frame& Frames::operator[](size_t i)
  {
    assert(i < frames.size());
    return *frames[i];
  }

  //
  // end getters

  void Frames::log_solvers() const
  {
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
    if (ctx.delta)
    {
      SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}", delta_solver->as_str());
    }
    else
      for (const std::unique_ptr<Frame>& f : frames)
      {
        SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}", f->get_solver().as_str());
      }
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
  }

  std::string Frames::blocked_str() const
  {
    std::string str;
    for (auto& f : frames)
    {
      str += f->blocked_str();
      str += '\n';
    }
    return str;
  }
  std::string Frames::solvers_str() const
  {
    std::string str;
    if (ctx.delta)
      str += delta_solver->as_str();
    else
    {
      for (auto& f : frames)
      {
        str += f->get_solver().as_str();
        str += '\n';
      }
    }
    return str;
  }

} // namespace pdr
