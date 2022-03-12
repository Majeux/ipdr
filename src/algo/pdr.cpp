#include "pdr.h"
#include "TextTable.h"
#include "output.h"
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
  PDR::PDR(context& c, Logger& l, PDResults& r)
      : ctx(c), logger(l), frames(ctx, logger), results(r)
  {
  }

  void PDR::reset()
  {
    logger.indent = 0;
    // trace is already converted into string, discard states
    assert(results.current().trace_string != "");
    results.current().trace.reset();
    frames_string     = "None";
    solvers_string    = "None";
    shortest_strategy = UINT_MAX;
  }

  void PDR::print_model(const z3::model& m)
  {
    logger("model consts \{");
    for (unsigned i = 0; i < m.num_consts(); i++)
      logger.out() << "\t" << m.get_const_interp(m.get_const_decl(i));
    logger("}");
  }

  bool PDR::run(bool optimize)
  {
    dynamic_cardinality = optimize;
    timer.reset();

    assert(k == frames.frontier());
    log_start();

    bool failed = false;
    if (!optimize || k == 0)
    {
      logger.show("Start initiation");
      logger.indent++;
      failed = !init();
      logger.indent--;
    }

    if (failed)
    {
      logger.show("Failed initiation");
      return finish(false);
    }

    logger.show("Survived Initiation");
    logger.show("Start iteration");
    logger.indent++;
    failed = !iterate();
    logger.indent--;

    if (failed)
    {
      logger.show("Failed iteration");
      return finish(false);
    }

    logger.show("Property verified");
    return finish(true);
  }

  bool PDR::finish(bool rv)
  {
    double final_time = timer.elapsed().count();
    logger.show(fmt::format("Total elapsed time {}", final_time));
    results.current().total_time = final_time;
    logger.stats.elapsed         = final_time;
    store_result();
    store_frame_strings();
    shortest_strategy = results.current().pebbles_used;

    return rv;
  }

  // returns true if the model survives initiation
  bool PDR::init()
  {
    assert(frames.frontier() == 0);

    z3::expr_vector notP = ctx.const_model().n_property.currents();
    if (frames.init_solver.check(notP))
    {
      logger.out() << "I =/> P" << std::endl;
      z3::model counter = frames.get_solver(0).get_model();
      print_model(counter);
      // TODO TRACE
      results.current().trace =
          std::make_shared<State>(ctx.const_model().get_initial());
      return false;
    }

    z3::expr_vector notP_next = ctx.const_model().n_property.nexts();
    if (frames.SAT(0, notP_next))
    { // there is a transitions from I to !P
      logger("I & T =/> P'");
      z3::model witness        = frames.get_solver(0).get_model();
      z3::expr_vector bad_cube = Solver::filter_witness(
          witness, [this](const z3::expr& e)
          { return ctx.const_model().literals.atom_is_current(e); });
      results.current().trace = std::make_shared<State>(bad_cube);

      return false;
    }

    frames.extend();
    k = 1;
    assert(k == frames.frontier());

    return true;
  }

  bool PDR::iterate()
  {
    logger(SEP3);
    logger("Start iteration");

    // I => P and I & T ⇒ P' (from init)
    while (true) // iterate over k, if dynamic this continues from last k
    {
      log_iteration();
      assert(k == frames.frontier());
      // exhaust all counters to the inductiveness of !P
      while (frames.trans_source(k, ctx.const_model().n_property.nexts(), true))
      {
        // a F_i state leads to violation
        z3::expr_vector cti = frames.get_solver(k).witness_current();
        log_cti(cti);

        z3::expr_vector core(ctx());
        int n = highest_inductive_frame(cti, (int)k - 1, (int)k, core);
        assert(n >= 0);

        // !s is inductive relative to F_n
        z3::expr_vector smaller_cti = generalize(core, n);
        frames.remove_state(smaller_cti, n + 1);

        if (not block(cti, n + 1, k))
          return false;

        logger.out() << std::endl;
      }
      SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| no more counters at F_{}",
                          logger.tab(), k);

      frames.extend();

      sub_timer.reset();
      int invariant_level = frames.propagate(k);
      double time         = sub_timer.elapsed().count();
      log_propagation(k, time);

      k++;
      frames.log_solvers();

      if (invariant_level >= 0)
      {
        results.current().invariant_index = invariant_level;
        return true;
      }
    }
  }

  bool PDR::block(z3::expr_vector& cti, unsigned n, unsigned level)
  {
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| block", logger.tab());
    logger.indent++;

    unsigned period = 0;
    std::set<Obligation, std::less<Obligation>> obligations;
    if ((n + 1) <= level)
      obligations.emplace(n + 1, std::move(cti), 0);

    // forall (n, state) in obligations: !state->cube is inductive
    // relative to F[i-1]
    while (obligations.size() > 0)
    {
      sub_timer.reset();
      double elapsed;
      std::string branch;

      auto [n, state, depth] = *(obligations.begin());
      assert(n <= level);
      log_top_obligation(obligations.size(), n, state->cube);

      // !state -> !state
      if (!frames.inductive(state->cube, n))
      {
        // get predecessor to state
        z3::expr_vector pred_cube = frames.get_solver(n).witness_current();

        std::shared_ptr<State> pred = std::make_shared<State>(pred_cube, state);
        log_pred(pred->cube);

        // state is at least inductive relative to F_n-2
        z3::expr_vector core(ctx());
        int m = highest_inductive_frame(pred->cube, n - 1, level, core);
        // n-1 <= m <= level
        if (m >= 0)
        {
          z3::expr_vector smaller_pred = generalize(core, m);
          frames.remove_state(smaller_pred, m + 1);

          if (static_cast<unsigned>(m + 1) <= level)
          {
            log_state_push(m + 1, pred->cube);
            obligations.emplace(m + 1, pred, depth + 1);
          }
        }
        else // intersects with I
        {
          results.current().trace = pred;
          return false;
        }
        elapsed = sub_timer.elapsed().count();
        branch  = "(pred)  ";
      }
      else
      {
        log_finish(state->cube);
        //! s is now inductive to at least F_n
        z3::expr_vector core(ctx());
        int m = highest_inductive_frame(state->cube, n + 1, level, core);
        // n <= m <= level
        assert(static_cast<unsigned>(m + 1) > n);

        if (m >= 0)
        {
          // !s is inductive to F_m
          z3::expr_vector smaller_state = generalize(core, m);
          // expr_vector smaller_state = generalize(state->cube, m);
          frames.remove_state(smaller_state, m + 1);
          obligations.erase(obligations.begin());

          if (static_cast<unsigned>(m + 1) <= level)
          {
            // push upwards until inductive relative to F_level
            log_state_push(m + 1, state->cube);
            obligations.emplace(m + 1, state, depth);
          }
        }
        else
        {
          results.current().trace = state;
          return false;
        }
        elapsed = sub_timer.elapsed().count();
        branch  = "(finish)";
      }
      log_obligation(branch, level, elapsed);
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
    return true;
  }

  void PDR::store_frame_strings()
  {
    std::stringstream ss;

    ss << "Frames" << std::endl;
    ss << frames.blocked_str() << std::endl;

    frames_string = ss.str();

    ss = std::stringstream();

    ss << "Solvers" << std::endl;
    ss << frames.solvers_str() << std::endl;

    solvers_string = ss.str();
  }

  void PDR::show_solver(std::ostream& out, unsigned it) const
  {
    out << SEP3 << " iteration " << it << std::endl;
    out << frames_string << std::endl;
    out << SEP2 << std::endl;
    out << solvers_string << std::endl;
  }

  void PDR::show_results(std::ostream& out) const { results.show(out); }

  void PDR::store_result()
  {
    if (std::shared_ptr<State> current = results.current().trace)
    {
      std::stringstream ss("Strategy:\n");
      TextTable t(' ');

      auto count_pebbled = [](const z3::expr_vector& vec)
      {
        unsigned count = 0;
        for (const z3::expr& e : vec)
          if (!e.is_not())
            count++;

        return count;
      };

      std::vector<std::string> initial_row =
          z3ext::to_strings(ctx.const_model().get_initial());
      initial_row.insert(initial_row.begin(), "No. pebbled = 0 |");
      initial_row.insert(initial_row.begin(), "I |");
      t.addRow(initial_row);

      unsigned i = 0;
      while (current)
      {
        i++;
        int pebbles = count_pebbled(current->cube);
        results.current().pebbles_used =
            std::max(results.current().pebbles_used, pebbles);

        std::vector<std::string> row = z3ext::to_strings(current->cube);
        row.insert(row.begin(), fmt::format("No. pebbled = {} |", pebbles));
        row.insert(row.begin(), std::to_string(i) + " |");
        t.addRow(row);

        current = current->prev;
      }

      std::vector<std::string> final_row =
          z3ext::to_strings(ctx.const_model().n_property.currents());
      final_row.insert(
          final_row.begin(),
          fmt::format("No. pebbled = {} |", ctx.const_model().get_f_pebbles()));
      final_row.insert(final_row.begin(), "F |");
      t.addRow(final_row);

      results.current().trace_length = i + 1;

      for (unsigned i = 0; i < t.rows()[0].size(); i++)
        t.setAlignment(i, TextTable::Alignment::RIGHT);
      ss << t;
      results.current().trace_string = ss.str();
    }
  }

  void PDR::show_trace(const std::shared_ptr<State> trace_root,
                       std::ostream& out) const
  {
    std::vector<std::tuple<unsigned, std::string, unsigned>> steps;

    std::shared_ptr<State> current = trace_root;
    auto count_pebbled             = [](const z3::expr_vector& vec)
    {
      unsigned count = 0;
      for (const z3::expr& e : vec)
        if (!e.is_not())
          count++;

      return count;
    };

    unsigned i = 0;
    while (current)
    {
      i++;
      steps.emplace_back(i, z3ext::join_expr_vec(current->cube),
                         count_pebbled(current->cube));
      current = current->prev;
    }
    unsigned i_padding = i / 10 + 1;

    out << fmt::format("{:>{}} |\t [ {} ]", 'I', i_padding,
                       z3ext::join_expr_vec(ctx.const_model().get_initial()))
        << std::endl;

    for (const auto& [num, vec, count] : steps)
      out << fmt::format("{:>{}} |\t [ {} ] No. pebbled = {}", num, i_padding,
                         vec, count)
          << std::endl;

    out << fmt::format(
               "{:>{}} |\t [ {} ]", 'F', i_padding,
               z3ext::join_expr_vec(ctx.const_model().n_property.currents()))
        << std::endl;
  }

  Statistics& PDR::stats() { return logger.stats; }
  int PDR::length_shortest_strategy() const { return shortest_strategy; }

} // namespace pdr
