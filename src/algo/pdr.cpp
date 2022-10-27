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

  PDR::PDR(Context& c, IModel& m, Logger& l)
      : ctx(c), model(m), logger(l), frames(ctx, model, logger)
  {
  }

  const Context& PDR::get_ctx() const { return ctx; }
  Context& PDR::get_ctx() { return ctx; }

  void PDR::reset() { frames.reset(); }

  void PDR::print_model(const z3::model& m)
  {
    logger.show("model consts \{");
    for (unsigned i = 0; i < m.num_consts(); i++)
      logger.show("\t{}", m.get_const_interp(m.get_const_decl(i)).to_string());
    logger.show("}");
  }

  PdrResult PDR::run()
  {
    timer.reset();
    log_start();

    if (frames.frontier() == 0)
    {
      logger.indent++;
      PdrResult init_res = init();
      logger.indent--;
      if (!init_res)
      {
        logger.and_whisper("Failed initiation");
        return finish(std::move(init_res));
      }
    }

    logger.and_show("\nStart iteration");
    logger.indent++;
    if (PdrResult it_res = iterate_short())
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

  PdrResult PDR::finish(PdrResult&& rv)
  {
    double final_time = timer.elapsed().count();
    logger.and_show(format("Total elapsed time {}", final_time));
    if (rv)
      logger.and_show("Invariant found");
    else
      logger.and_show("Terminated with trace");

    rv.time = final_time;

    logger.stats.elapsed = final_time;
    logger.stats.write(model.constraint_str());
    logger.stats.write();
    logger.stats.clear();
    store_frame_strings();
    logger.indent = 0;

    logger.and_show("final solver");
    logger.and_show(frames.blocked_str());

    return rv;
  }

  // returns true if the model survives initiation
  PdrResult PDR::init()
  {
    logger.and_whisper("Start initiation");
    assert(frames.frontier() == 0);

    if (frames.init_solver.check(model.n_property))
    {
      logger.whisper("I =/> P");
      return PdrResult::found_trace(model.get_initial());
    }

    if (frames.SAT(0, model.n_property.p()))
    { // there is a transitions from I to !P
      logger.show("I & T =/> P'");
      expr_vector bad_cube = frames.get_solver(0).witness_current();
      return PdrResult::found_trace(bad_cube);
    }

    frames.extend();

    logger.and_whisper("Survived Initiation");
    return PdrResult::empty_true();
  }

  PdrResult PDR::iterate()
  {
    // I => P and I & T ⇒ P' (from init)
    if (ctx.type != Tactic::decrement)
      assert(frames.frontier() == 1);

    for (size_t k = frames.frontier(); true; k++, frames.extend())
    {
      log_iteration();
      logger.whisper("Iteration k={}", k);
      while (optional<z3ext::solver::Witness> cti =
                 frames.get_trans_source(k, model.n_property.p(), true))
      {
        log_cti(cti->curr, k); // cti is an F_i state that leads to a violation

        auto [n, core] = highest_inductive_frame(cti->curr, k - 1);
        // assert(n >= 0);

        // !s is inductive relative to F_n
        expr_vector sub_cube = generalize(core, n);
        frames.remove_state(sub_cube, n + 1);

        PdrResult res = block(cti->curr, n);
        if (not res)
        {
          logger.and_show("Terminated with trace");
          return res;
        }

        logger.show("");
      }
      logger.tabbed("no more counters at F_{}", k);

      sub_timer.reset();

      optional<size_t> invariant_level = frames.propagate();
      double time                      = sub_timer.elapsed().count();
      log_propagation(k, time);
      frames.log_solvers(true);

      if (invariant_level)
        return PdrResult::found_invariant(*invariant_level);
    }
  }

  PdrResult PDR::block(expr_vector cti, unsigned n)
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
      // logger(frames.solvers_str(true));
      // logger(frames.blocked_str());
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
        std::shared_ptr<PdrState> pred =
            std::make_shared<PdrState>(*pred_cube, state);
        log_pred(pred->cube);

        // state is at least inductive relative to F_n-2
        // z3::expr_vector core(ctx());
        auto [m, core] = highest_inductive_frame(pred->cube, n - 1);
        // n-1 <= m <= level
        if (m < 0) // intersects with I
          return PdrResult::found_trace(pred);

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
          return PdrResult::found_trace(state);

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
    return PdrResult::empty_true();
  }

  void PDR::store_frame_strings()
  {
    using std::endl;
    std::stringstream ss;

    ss << SEP3 << endl
       << "# " << model.constraint_str() << endl
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

} // namespace pdr
