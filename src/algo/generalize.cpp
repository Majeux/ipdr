#include <algorithm>
#include <cstddef>
#include <dbg.h>
#include <fmt/core.h>
#include <spdlog/stopwatch.h>
#include <vector>
#include <z3++.h>

#include "logger.h"
#include "pdr.h"
#include "string-ext.h"
#include "z3-ext.h"

namespace pdr
{
  using std::optional;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  bool ev_str_eq(const z3::expr_vector& ev, const vector<std::string>& str)
  {
    if (ev.size() != str.size())
      return false;

    for (size_t i = 0; i < ev.size(); i++)
      if (ev[i].to_string() != str[i])
        return false;
    return true;
  }

  //! s is inductive up until min-1. !s is included up until min
  PDR::HIFresult PDR::hif_(const expr_vector& cube, int min)
  {
    int max = frames.frontier();
    if (min <= 0 && !frames.inductive(cube, 0))
    {
      MYLOG_DEBUG(logger, "Intersects I");
      return { -1, {} };
    }

    // F_result & !cube & T & cube' = UNSAT
    // => F_result & !cube & T & core' = UNSAT
    optional<expr_vector> raw_core;

    int highest = max;
    for (int i = std::max(1, min); i <= max; i++)
    {
      // clause was inductive up to this iteration
      if (!frames.inductive(cube, i))
      {
        highest = i - 1; // previous was greatest inductive frame
        break;
      }
      raw_core = frames.get_solver(i).unsat_core();
    }

    MYLOG_DEBUG(logger, "highest inductive frame is {} / {}", highest,
        frames.frontier());
    return { highest, raw_core };
  }

  PDR::HIFresult PDR::highest_inductive_frame(const expr_vector& cube, int min)
  {
    expr_vector rv_core(ctx());
    HIFresult result = hif_(cube, min);

    if (result.level >= 0 && result.level >= min && result.core)
    { // if unsat result occurs
      auto next_lits  = [this](const expr& e) { return ts.vars.lit_is_p(e); };
      auto to_current = [this](const expr& e) { return ts.vars(e); };
      rv_core = z3ext::filter_transform(*result.core, next_lits, to_current);

      MYLOG_DEBUG(logger, "core @{}: [{}]", result.level,
          rv_core ? z3ext::join_ev(rv_core) : "none");

      // if I => !core, the subclause survives initiation and is inductive
      if (frames.init_solver.check(rv_core) == z3::sat)
      {
        MYLOG_DEBUG(logger, "unsat core is invalid. no reduction.");
        rv_core = cube; /// I /=> !core, use original
      }
      else
      {
        MYLOG_DEBUG(logger, "unsat core reduction: {} -> {}", cube.size(),
            rv_core.size());
      }
    }
    else
      rv_core = cube; // no core produced

    MYLOG_TRACE(logger, "new cube: [{}]", join_expr_vec(rv_core, false));
    return { result.level, rv_core };
  }

  expr_vector PDR::generalize(const expr_vector& state, int level)
  {
    MYLOG_DEBUG(logger, "generalize cube");
    MYLOG_TRACE(logger, "[{}]", join_expr_vec(state, false));

    logger.indent++;
    spdlog::stopwatch timer;
    IF_STATS(double s0 = state.size());

    expr_vector smaller_cube = MIC(state, level);

    IF_STATS({
      logger.stats.generalization.add(level, timer.elapsed().count());
      double reduction = (s0 - smaller_cube.size()) / s0;
      logger.stats.generalization_reduction.add(reduction);
    });
    logger.indent--;

    MYLOG_DEBUG(
        logger, "generalization: {} -> {}", state.size(), smaller_cube.size());
    MYLOG_TRACE(logger, "final reduced cube = [{}]",
        join_expr_vec(smaller_cube, false));

    return smaller_cube;
  }

  expr_vector PDR::MIC(const expr_vector& state, int level)
  {
    assert(level <= (int)frames.frontier());
    // used for sorting
    vector<expr> cube = z3ext::convert(state);

    unsigned attempts{ 0u };
    for (unsigned i{ 0 }; i < cube.size();)
    {
      assert(z3ext::lits_ordered(cube));
      vector<expr> new_cube(cube.begin(), cube.begin() + i);
      new_cube.reserve(cube.size() - 1);
      new_cube.insert(new_cube.end(), cube.begin() + i + 1, cube.end());

      MYLOG_TRACE(
          logger, "verifying subcube [{}]", join_expr_vec(new_cube, false));

      logger.indent++;
      if (down(new_cube, level))
      {
        MYLOG_TRACE(logger, "sub-cube survived");
        MYLOG_TRACE(logger, "down-reduced cube ({} -> {}): [{}]", cube.size(),
            new_cube.size(), join_expr_vec(new_cube));
        // current literal was dropped, i now points to the next
        cube = std::move(new_cube);
#warning try difference between tracking attempts per clause or no. times in a row
        // attempts = 0;
      }
      else
      {
        MYLOG_TRACE(logger, "sub-cube failed");
        i++;
      }
      logger.indent--;

      attempts++;
      if (attempts >= ctx.mic_retries)
      {
        IF_STATS(logger.stats.mic_limit++;);
        MYLOG_WARN(logger, "MIC exceeded {} attempts", ctx.mic_retries);
        break;
      }
    }
    IF_STATS(logger.stats.mic_attempts.add(attempts));

    return z3ext::convert(cube);
  }

  // state is sorted
  bool PDR::down(vector<expr>& state, int level)
  {

    while (true)
    {
      assert(z3ext::lits_ordered(state));
      expr* const raw_state = state.data();
      if (frames.init_solver.check(state.size(), raw_state) == z3::sat)
      {
        MYLOG_TRACE(logger, "state includes I");
        return false;
      }

      if (!frames.inductive(state, level))
      {
        MYLOG_TRACE(logger, "state is not inductive");
        MYLOG_TRACE(logger, "intersect with witness");
        logger.indent++;
        state = frames.get_solver(level).witness_current_intersect(state);
        logger.indent--;
        MYLOG_TRACE(
            logger, "new intersected state -> [{}]", z3ext::join_ev(state));
      }
      else
        return true;
    }
    return false;
  }
} // namespace pdr
