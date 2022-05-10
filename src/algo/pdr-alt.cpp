#include "pdr.h"
#include "string-ext.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <z3++.h>

namespace pdr
{
  using std::make_shared;
  using std::optional;
  using std::shared_ptr;
  using std::string;
  using z3::expr;
  using z3::expr_vector;

  Result PDR::iterate_short()
  {
    // I => P and I & T ⇒ P' (from init)
    if (ctx.type != Tactic::decrement) // decr continues from last level
      assert(frames.frontier() == 1);

    expr_vector notP_next = ctx.model().n_property.nexts();
    for (size_t k = frames.frontier(); true; k++, frames.extend())
    {
      log_iteration();
      logger.whisper("Iteration k={}", k);
      while (std::optional<expr_vector> cti =
                 frames.get_trans_source(k, notP_next, true))
      {
        log_cti(*cti, k); // cti is an F_i state that leads to a violation

        Result res = block_short(std::move(*cti), k - 1); // is cti reachable from F_k-1 ?
        if (not res)
          return res;

        logger.show("");
      }
      logger.tabbed("no more counters at F_{}", k);

      sub_timer.reset();

      optional<size_t> invariant_level = frames.propagate();
      double time                      = sub_timer.elapsed().count();
      log_propagation(k, time);

      if (invariant_level)
        return Result::found_invariant(frames.max_pebbles, *invariant_level);
    }
  }

  Result PDR::block_short(expr_vector&& cti, unsigned n)
  {
    unsigned k = frames.frontier();
    logger.tabbed("block");
    logger.indent++;

    if (ctx.type != Tactic::increment)
    {
      logger.tabbed_and_whisper("Cleared obligations.");
      obligations.clear();
    }
    else
      logger.tabbed_and_whisper("Reused obligations.");

    unsigned period = 0;
    if (n <= k)
      obligations.emplace(n, std::move(cti), 0);

    // forall (n, state) in obligations: !state->cube is inductive
    // relative to F[i-1]
    while (obligations.size() > 0)
    {
      sub_timer.reset();
      double elapsed;
      string branch;

      auto [n, state, depth] = *(obligations.begin());
      assert(n <= k);
      log_top_obligation(obligations.size(), n, state->cube);

      // !state -> state
      if (optional<expr_vector> pred_cube =
              frames.counter_to_inductiveness(state->cube, n))
      {
        shared_ptr<State> pred = make_shared<State>(*pred_cube, state);
        log_pred(pred->cube);

        if (n == 0) // intersects with I
          return Result::found_trace(frames.max_pebbles, pred);

        obligations.emplace(n - 1, pred, depth + 1);

        elapsed = sub_timer.elapsed().count();
        branch  = "(pred)  ";
      }
      else
      {
        log_finish(state->cube);
        //! s is now inductive to at least F_n
        #warning TODO: try generalizing before pushing
        auto [m, core] = highest_inductive_frame(state->cube, n + 1);
        // n <= m <= level
        assert(static_cast<unsigned>(m + 1) > n);

        if (m < 0)
          return Result::found_trace(frames.max_pebbles, state);

        // !s is inductive to F_m
        expr_vector smaller_state = generalize(core, m);
        frames.remove_state(smaller_state, m + 1);
        obligations.erase(obligations.begin());

        if (static_cast<unsigned>(m + 1) <= k)
        {
          // push upwards until inductive relative to F_level
          log_state_push(m + 1);
          obligations.emplace(m + 1, state, depth);
        }

        elapsed = sub_timer.elapsed().count();
        branch  = "(finish)";
      }
      log_obligation_done(branch, k, elapsed);
      elapsed = -1.0;
    }

    logger.indent--;
    return Result::empty_true();
  }
} // namespace pdr
