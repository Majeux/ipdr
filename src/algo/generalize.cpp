#include <algorithm>
#include <cstddef>
#include <fmt/core.h>
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
    // auto next_lits  = [this](const expr& e) { return ts.vars.lit_is_p(e); };
    // auto to_current = [this](const expr& e) { return ts.vars(e); };

    optional<expr_vector> core;

    int highest = max;
    for (int i = std::max(1, min); i <= max; i++)
    {
      // clause was inductive up to this iteration
      if (!frames.inductive(cube, i))
      {
        core = z3::expr_vector(ctx);
        if (core)
        {
          MYLOG_DEBUG(
              logger, "core @{}: [{}]", i - 1, z3ext::join_ev(core.value()));
        }
        else
        {
          MYLOG_DEBUG(logger, "no core");
        }

        highest = i - 1; // previous was greatest inductive frame
        break;
      }
      // core = frames.get_solver(i).unsat_core(next_lits, to_current);
    }

    MYLOG_DEBUG(logger, "highest inductive frame is {} / {}", highest,
        frames.frontier());
    return { highest, core };
  }

  PDR::HIFresult PDR::highest_inductive_frame(const expr_vector& cube, int min)
  {
    expr_vector rv_core(ctx());
    // const HIFresult result = hif_(cube, min);
    HIFresult result = hif_(cube, min);

    // if (result.level >= 0 && result.level >= min &&
    //     result.core) // if unsat result occurs
    // {
    //   auto next_lits  = [this](const expr& e) { return ts.vars.lit_is_p(e); };
    //   auto to_current = [this](const expr& e) { return ts.vars(e); };
    //   result.core =
    //       frames.get_solver(result.level).unsat_core(next_lits, to_current);
    //   // if (result.core->size() == 0)
    //   // {
    //   //   MYLOG_WARN(logger, "0 core at level {}", result.level);
    //   //   frames.log_solver(true);
    //   // }
    //   // if I => !core, the subclause survives initiation and is inductive
    //   if (frames.init_solver.check(*result.core) == z3::sat)
    //   {
    //     MYLOG_DEBUG(logger, "unsat core is invalid");
    //     rv_core = cube; /// I /=> !core, use original
    //   }
    //   else
    //   {
    //     MYLOG_DEBUG(logger, "unsat core reduction: {} -> {}", cube.size(),
    //         rv_core.size());
    //     rv_core = *result.core;
    //   }
    // }
    // else
    rv_core = cube; // no core produced

    // MYLOG_DEBUG(logger, "new cube: [{}]", join_expr_vec(rv_core, false));
    return { result.level, rv_core };
  }

  expr_vector PDR::generalize(const expr_vector& state, int level)
  {
    MYLOG_DEBUG(logger, "generalize cube");
    // MYLOG_DEBUG(logger, "[{}]", join_expr_vec(state, false));
    logger.indent++;
    expr_vector smaller_cube = MIC(state, level);
    logger.indent--;

    MYLOG_DEBUG(
        logger, "generalization: {} -> {}", state.size(), smaller_cube.size());
    // MYLOG_DEBUG(
    // logger, "final reduced cube = [{}]", join_expr_vec(smaller_cube, false));
    return smaller_cube;
  }

  expr_vector PDR::MIC(const expr_vector& state, int level)
  {
    assert(level <= (int)frames.frontier());
    // used for sorting
    vector<expr> cube = z3ext::convert(state);

    unsigned attempts = 0;
    for (unsigned i = 0; i < cube.size();)
    {
      assert(z3ext::lits_ordered(cube));
      if (attempts > ctx.mic_retries)
      {
        MYLOG_WARN(logger, "MIC exceeded {} attempts", ctx.mic_retries);
        break;
      }
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
#warning try difference between tracking attempts per clause or per literal
        attempts = 0;
      }
      else
      {
        MYLOG_TRACE(logger, "sub-cube failed");
        i++;
        attempts++;
      }
      logger.indent--;
    }

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
            logger, "new intersected state -> [{}]", join_expr_vec(state));
      }
      else
        return true;
    }
    return false;
  }
} // namespace pdr
