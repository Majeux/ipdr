#include <algorithm>
#include <cstddef>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <z3++.h>

#include "pdr.h"
#include "string-ext.h"
#include "z3-ext.h"

namespace pdr
{
  using std::optional;
  using std::pair;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3ext::join_expr_vec;

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
      logger.tabbed("Intersects I");
      return { -1, {} };
    }

    // F_result & !cube & T & cube' = UNSAT
    // => F_result & !cube & T & core' = UNSAT
    auto next_lits  = [this](const expr& e) { return model.vars.lit_is_p(e); };
    auto to_current = [this](const expr& e) { return model.vars(e); };

    optional<expr_vector> core;

    int highest = max;
    for (int i = std::max(1, min); i <= max; i++)
    {
      // clause was inductive up to this iteration
      if (!frames.inductive(cube, i))
      {
        highest = i - 1; // previous was greatest inductive frame
        break;
      }
      core = frames.get_solver(i).unsat_core(next_lits, to_current);
    }


    logger.tabbed(
        "highest inductive frame is {} / {}", highest, frames.frontier());
    return { highest, core };
  }

  PDR::HIFresult PDR::highest_inductive_frame(const expr_vector& cube, int min)
  {
    expr_vector rv_core(ctx());
    const HIFresult result = hif_(cube, min);
    if (result.level >= 0 && result.level >= min && result.core) // if unsat result occurs
    {
      if (result.core->size() == 0)
      {
        logger.warn("0 core at level {}", result.level);
        frames.log_solver(true);
      }
      // if I => !core, the subclause survives initiation and is inductive
      if (frames.init_solver.check(*result.core) == z3::sat)
        rv_core = cube; /// I /=> !core, use original
      else
        rv_core = *result.core;
    }
    else
      rv_core = cube; // no core produced

    logger.tabbed("cube reduction: {} -> {}", cube.size(), rv_core.size());
    logger.tabbed_trace("new cube: [{}]", join_expr_vec(rv_core, false));
    return { result.level, rv_core };
  }

  expr_vector PDR::generalize(const expr_vector& state, int level)
  {
    logger.tabbed("generalize cube");
    logger.tabbed_trace("[{}]", join_expr_vec(state, false));
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

    assert(std::is_sorted(cube.begin(), cube.end(), z3ext::lit_less()));
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

      logger("verifying subcube [{}]", z3ext::join_expr_vec(new_cube, false));

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
    assert(std::is_sorted(state.begin(), state.end(), z3ext::lit_less()));

    while (true)
    {
      expr* const raw_state = state.data();
      if (frames.init_solver.check(state.size(), raw_state) == z3::sat)
        return false;

      if (!frames.inductive(state, level))
        state = frames.get_solver(level).witness_current_intersect(state);
      else
        return true;
    }
    return false;
  }
} // namespace pdr
