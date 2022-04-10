#include <algorithm>
#include <cstddef>
#include <vector>
#include <z3++.h>

#include "pdr.h"
#include "string-ext.h"
#include "z3-ext.h"

namespace pdr
{
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  //! s is inductive up until min-1. !s is included up until min
  int PDR::hif_(const expr_vector& cube, int min)
  {
    int max = frames.frontier();
    if (min <= 0 && !frames.inductive(cube, 0))
    {
      logger.tabbed("Intersects I");
      return -1;
    }

    int highest = max;
    for (int i = std::max(1, min); i <= max; i++)
    {
      // clause was inductive up to this iteration
      if (!frames.inductive(cube, i))
      {
        highest = i - 1; // previous was greatest inductive frame
        break;
      }
    }

    logger.tabbed("highest inductive frame is {}", highest);
    return highest;
  }

  std::tuple<int, expr_vector> PDR::highest_inductive_frame(
      const expr_vector& cube, int min)
  {
    expr_vector core(ctx());
    int result = hif_(cube, min);
    if (result >= 0 && result >= min) // if unsat result occurs
    {
      // F_result & !cube & T & cube' = UNSAT
      // => F_result & !cube & T & core' = UNSAT
      auto next_lits = [this](const expr& e)
      { return ctx.model().lits.literal_is_p(e); };
      auto to_current = [this](const expr& e)
      { return ctx.model().lits(e); };

      core = frames.get_solver(result).unsat_core(next_lits, to_current);

      // if I => !core, the subclause survives initiation and is inductive
      if (frames.init_solver.check(core) == z3::sat)
        core = cube; /// I /=> !core, use original
    }
    else
      core = cube; // no core produced

    logger.tabbed("cube reduction: {} -> {}", cube.size(), core.size());
    return { result, core };
  }

  expr_vector PDR::generalize(const expr_vector& state, int level)
  {
    logger.tabbed("generalize cube");
    logger.indent++;
    expr_vector smaller_cube = MIC(state, level);
    logger.indent--;

    logger.tabbed(
        "reduction by MIC: {} -> {}", state.size(), smaller_cube.size());
    // SPDLOG_LOGGER_TRACE(log, "{}| final reduced cube = [{}]", TAB,
    // join(smaller_cube));
    return smaller_cube;
  }

  expr_vector PDR::MIC(const expr_vector& state, int level)
  {
    assert(level <= (int)frames.frontier());
    // used for sorting
    vector<expr> cube = z3ext::convert(state);

    assert(std::is_sorted(cube.begin(), cube.end(), z3ext::expr_less()));
    unsigned attempts = 0;
    for (unsigned i = 0; i < cube.size();)
    {
      if (attempts > mic_retries)
      {
        logger("MIC exceeded {} attempts", mic_retries);
        break;
      }
      vector<expr> new_cube(cube.begin(), cube.begin() + i);
      new_cube.reserve(cube.size() - 1);
      new_cube.insert(new_cube.end(), cube.begin() + i + 1, cube.end());

      logger.indent++;
      if (down(new_cube, level))
      {
        // current literal was dropped, i now points to the next
        cube     = std::move(new_cube);
        attempts = 0;
        // SPDLOG_LOGGER_TRACE(log, "{}| reduced cube: [{}]", TAB,
        // join(cube));
      }
      else
      {
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
    assert(std::is_sorted(state.begin(), state.end(), z3ext::expr_less()));
    auto is_current_in_state = [this, &state](const expr& e)
    {
      return ctx.model().lits.literal_is_current(e) &&
             std::binary_search(
                 state.begin(), state.end(), e, z3ext::expr_less());
    };

    while (true)
    {
      expr* const raw_state = state.data();
      if (frames.init_solver.check(state.size(), raw_state) == z3::sat)
        return false;

      if (!frames.inductive(state, level))
      {
        // intersect the current states from the model with state
        z3::model witness = frames.get_solver(level).get_model();
        vector<expr> cti_intersect =
            Solver::filter_witness_vector(witness, is_current_in_state);

        state = std::move(cti_intersect);
      }
      else
        return true;
    }
    return false;
  }
} // namespace pdr
