#include "pdr.h"
#include "TextTable.h"
#include "output.h"
#include "result.h"
#include "solver.h"
#include "stats.h"
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
  using fmt::format;
  using std::optional;
  using std::string;
  using z3::expr;
  using z3::expr_vector;

  PDR::PDR(Context& c, Logger& l) : ctx(c), logger(l), frames(ctx, logger) {}

  const Context& PDR::get_ctx() const { return ctx; }
  Context& PDR::get_ctx() { return ctx; }

  void PDR::reset() { shortest_strategy = UINT_MAX; }

  void PDR::reconstrain(unsigned x) { frames.reset_constraint(x); }

  void PDR::print_model(const z3::model& m)
  {
    logger.show("model consts \{");
    for (unsigned i = 0; i < m.num_consts(); i++)
      logger.show("\t{}", m.get_const_interp(m.get_const_decl(i)).to_string());
    logger.show("}");
  }

  Result PDR::_run()
  {
    timer.reset();
    // TODO run type preparation logic here
    log_start();

    if (frames.frontier() == 0)
    {
      logger.indent++;
      Result init_res = init();
      logger.indent--;
      if (!init_res)
      {
        logger.and_whisper("Failed initiation");
        return finish(std::move(init_res));
      }
    }

    logger.and_show("\nStart iteration");
    logger.indent++;
    if (Result it_res = iterate())
    {
      logger.and_whisper("Property verified");
      logger.indent--;
      return finish(std::move(it_res));
    }
    else
    {
      logger.and_whisper("Failed iteration");
      return finish(std::move(it_res));
    }
  }

  Result PDR::run(Tactic pdr_type, optional<unsigned> max_p)
  {
    ctx.type = pdr_type;
    frames.reset_constraint(max_p);
    return _run();
  }

  Result PDR::increment_run(unsigned max_p)
  {
    ctx.type = Tactic::increment;
    frames.increment_reset(max_p);
    return _run();
  }

  Result PDR::finish(Result&& rv)
  {
    double final_time = timer.elapsed().count();
    logger.and_show(format("Total elapsed time {}", final_time));
    rv.total_time = final_time;

    logger.stats.elapsed = final_time;
    logger.stats.write(constraint_str());
    logger.stats.write();
    logger.stats.clear();
    store_frame_strings();
    if (!rv)
      shortest_strategy = std::min(shortest_strategy, rv.trace().marked);
    logger.indent = 0;
	rv.finalize(ctx);

    return rv;
  }

  // returns true if the model survives initiation
  Result PDR::init()
  {
    logger.and_whisper("Start initiation");
    assert(frames.frontier() == 0);

    const PebblingModel& m = ctx.model();
    expr_vector notP       = m.n_property.currents();

    if (frames.init_solver.check(notP))
    {
      logger.whisper("I =/> P");
      return Result::found_trace(frames.max_pebbles, m.get_initial());
    }

    expr_vector notP_next = m.n_property.nexts();
    if (frames.SAT(0, notP_next))
    { // there is a transitions from I to !P
      logger.show("I & T =/> P'");
      expr_vector bad_cube = frames.get_solver(0).witness_current();
      return Result::found_trace(frames.max_pebbles, bad_cube);
    }

    frames.extend();

    logger.and_whisper("Survived Initiation");
    return Result::empty_true();
  }

  Result PDR::iterate()
  {
    // I => P and I & T ⇒ P' (from init)
    expr_vector notP_next = ctx.model().n_property.nexts();
    unsigned k            = 1;
    while (true) // iterate over k, if dynamic this continues from last k
    {
      log_iteration();
      // int k = frames.frontier();
      assert(k == frames.frontier());
      logger.whisper("Iteration k={}", k);
      while (std::optional<expr_vector> cti =
                 frames.get_trans_source(k, notP_next, true))
      {
        log_cti(*cti, k); // cti is an F_i state that leads to a violation

        auto [n, core] = highest_inductive_frame(*cti, k - 1);
        // assert(n >= 0);

        // !s is inductive relative to F_n
        expr_vector sub_cube = generalize(core, n);
        frames.remove_state(sub_cube, n + 1);

#warning try top down
        Result res = block(*cti, n);
        if (not res)
        {
          logger.and_show("Terminated with trace");
          return res;
        }

        logger.show("");
      }
      logger.tabbed("no more counters at F_{}", k);

      sub_timer.reset();

      int invariant_level = frames.propagate();
      double time         = sub_timer.elapsed().count();
      log_propagation(frames.frontier(), time);
      frames.log_solvers(true);

      if (invariant_level >= 0)
        return Result::found_invariant(frames.max_pebbles, invariant_level);
      frames.extend();
      k++;
    }
  }

  Result PDR::block(expr_vector cti, unsigned n)
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
    if ((n + 1) <= k)
      obligations.emplace(n + 1, std::move(cti), 0);

    // forall (n, state) in obligations: !state->cube is inductive
    // relative to F[i-1]
    while (obligations.size() > 0)
    {
      logger(frames.solvers_str(true));
      logger(frames.blocked_str());
      sub_timer.reset();
      double elapsed;
      string branch;

      auto [n, state, depth] = *(obligations.begin());
      assert(n <= k);
      log_top_obligation(obligations.size(), n, state->cube);

      // !state -> state
      if (std::optional<expr_vector> pred_cube =
              frames.counter_to_inductiveness(state->cube, n))
      {
        std::shared_ptr<State> pred =
            std::make_shared<State>(*pred_cube, state);
        log_pred(pred->cube);

        // state is at least inductive relative to F_n-2
        // z3::expr_vector core(ctx());
        auto [m, core] = highest_inductive_frame(pred->cube, n - 1);
        // n-1 <= m <= level
        if (m < 0) // intersects with I
          return Result::found_trace(frames.max_pebbles, pred);

        expr_vector smaller_pred = generalize(core, m);
        frames.remove_state(smaller_pred, m + 1);

        if (static_cast<unsigned>(m + 1) <= k)
        {
          log_state_push(m + 1);
          obligations.emplace(m + 1, pred, depth + 1);
        }

        elapsed = sub_timer.elapsed().count();
        branch  = "(pred)  ";
      }
      else
      {
        log_finish(state->cube);
        //! s is now inductive to at least F_n
        auto [m, core] = highest_inductive_frame(state->cube, n + 1);
        // n <= m <= level
        assert(static_cast<unsigned>(m + 1) > n);

        if (m < 0)
          return Result::found_trace(frames.max_pebbles, state);

        // !s is inductive to F_m
        expr_vector smaller_state = generalize(core, m);
        // expr_vector smaller_state = generalize(state->cube, m);
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

      // periodically write stats in case of long runs
      /*
      if (period >= 100)
      {
        period = 0;
        logger.out() << "Stats written" << std::endl;
        SPDLOG_LOGGER_DEBUG(logger.spd_logger, logger.stats.to_string());
        logger.spd_logger->flush();
      }
      else
        period++;
      */
    }

    logger.indent--;
    return Result::empty_true();
  }

  string PDR::constraint_str() const
  {
    if (frames.max_pebbles)
      return format("cardinality {}", *frames.max_pebbles);
    else
      return "no constraint";
  }

  void PDR::store_frame_strings()
  {
    using std::endl;
    std::stringstream ss;

    ss << SEP3 << endl
       << "# " << constraint_str() << endl
       << "Frames" << endl
       << frames.blocked_str() << endl
       << SEP2 << endl
       << "Solvers" << endl
       << frames.solvers_str(true) << endl;

    logger.stats.solver_dumps.push_back(ss.str());
  }

  void PDR::show_solver(std::ostream& out) const // TODO
  {
    for (const std::string& s : logger.stats.solver_dumps)
      out << s << std::endl << std::endl;
  }

  int PDR::length_shortest_strategy() const { return shortest_strategy; }

} // namespace pdr
