#include "pdr.h"
#include "TextTable.h"
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
  PDR::PDR(PDRModel& m, bool d, const std::string& log_file)
      : ctx(m.ctx), model(m), delta(d), logger(log_file),
        frames(delta, ctx, m, logger), results(1)
  {
  }

  void PDR::reset()
  {
    logger.indent = 0;
    result().trace.reset();
    logger.stats = Statistics();
    frames_string = "None";
    solvers_string = "None";
  }

  void PDR::print_model(const z3::model& m)
  {
    std::cout << "model consts \{" << std::endl;
    for (unsigned i = 0; i < m.num_consts(); i++)
      std::cout << "\t" << m.get_const_interp(m.get_const_decl(i));
    std::cout << "}" << std::endl;
  }

  bool PDR::run(bool optimize)
  {
    dynamic_cardinality = optimize;
    reset();
    timer.reset();

    assert(k == frames.frontier());

    bool failed = false;
    std::cout << std::endl;
    SPDLOG_LOGGER_INFO(logger.spd_logger, "");
    SPDLOG_LOGGER_INFO(logger.spd_logger, "NEW RUN\n");
    log_and_show("PDR start:");

    if (!optimize || k == 0)
    {
      SPDLOG_LOGGER_INFO(logger.spd_logger, "Start initiation");
      logger.indent++;
      failed = !init();
      logger.indent--;
    }

    if (failed)
    {
      log_and_show("Failed initiation");
      return finish(false);
    }
    log_and_show("Survived Initiation");

    SPDLOG_LOGGER_INFO(logger.spd_logger, "Start iteration");
    logger.indent++;
    failed = !iterate();
    logger.indent--;

    if (failed)
    {
      log_and_show("Failed iteration");
      return finish(false);
    }

    log_and_show("Property verified");
    return finish(true);
  }

  bool PDR::finish(bool rv)
  {
    double final_time = timer.elapsed().count();
    log_and_show(fmt::format("Total elapsed time {}", final_time));
	result().total_time = final_time;
    logger.stats.elapsed = final_time;
    store_result();
    store_frame_strings();
    if (dynamic_cardinality)
    {
      results.push_back(PDResult());
      result() = results.back();
    }

    return rv;
  }

  // returns true if the model survives initiation
  bool PDR::init()
  {
    assert(frames.frontier() == 0);

    SPDLOG_LOGGER_TRACE(logger.spd_logger, "Start initiation");
    z3::expr_vector notP = model.n_property.currents();
    if (frames.init_solver.check(notP))
    {
      std::cout << "I =/> P" << std::endl;
      z3::model counter = frames.solver(0)->get_model();
      print_model(counter);
      // TODO TRACE
      result().trace = std::make_shared<State>(model.get_initial());
      return false;
    }

    z3::expr_vector notP_next = model.n_property.nexts();
    if (frames.SAT(0, notP_next))
    { // there is a transitions from I to !P
      std::cout << "I & T =/> P'" << std::endl;
      z3::model witness = frames.solver(0)->get_model();
      z3::expr_vector bad_cube =
          Solver::filter_witness(witness, [this](const z3::expr& e)
                                 { return model.literals.atom_is_current(e); });
      result().trace = std::make_shared<State>(bad_cube);

      return false;
    }

    frames.extend();
    k = 1;
    assert(k == frames.frontier());

    return true;
  }

  bool PDR::iterate()
  {
    std::cout << SEP3 << std::endl;
    std::cout << "Start iteration" << std::endl;

    // I => P and I & T ⇒ P' (from init)
    while (true) // iterate over k, if dynamic this continues from last k
    {
      log_iteration();
      assert(k == frames.frontier());

      while (true) // exhaust all transitions to !P
      {
        Witness witness =
            frames.get_trans_from_to(k, model.n_property.nexts(), true);

        if (witness)
        {
          // F_i leads to violation, strengthen
          z3::expr_vector cti_current = Solver::filter_witness(
              *witness, [this](const z3::expr& e)
              { return model.literals.atom_is_current(e); });

          log_cti(cti_current);

          z3::expr_vector core(ctx);
          int n =
              highest_inductive_frame(cti_current, (int)k - 1, (int)k, core);
          assert(n >= 0);

          // F_n & T & !s => !s
          // F_n & T => F_n+1
          z3::expr_vector smaller_cti = generalize(core, n);
          frames.remove_state(smaller_cti, n + 1);

          if (not block(cti_current, n + 1, k))
            return false;

          std::cout << std::endl;
        }
        else // no more counter examples
        {
          SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| no more counters at F_{}",
                              logger.tab(), k);
          break;
        }
      }

      frames.extend();

      sub_timer.reset();
      int invariant = frames.propagate(k);
      double time = sub_timer.elapsed().count();
      log_propagation(k, time);

      k++;
      frames.log_solvers();

      if (invariant >= 0)
	  {
		result().invariant_index = invariant;
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

      if (Witness w = frames.counter_to_inductiveness(state->cube, n))
      {
        // get predecessor from the witness
        auto extract_current = [this](const z3::expr& e)
        { return model.literals.atom_is_current(e); };
        z3::expr_vector pred_cube = Solver::filter_witness(*w, extract_current);

        std::shared_ptr<State> pred = std::make_shared<State>(pred_cube, state);
        log_pred(pred->cube);

        // state is at least inductive relative to F[n-2]
        z3::expr_vector core(ctx);
        int m = highest_inductive_frame(pred->cube, n - 1, level, core);
        // m in [n-1, level]
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
          result().trace = pred;
          return false;
        }
        elapsed = sub_timer.elapsed().count();
        branch = "(pred)  ";
      }
      else
      {
        log_finish(state->cube);
        //! s is now inductive to at least F_n
        // see if !state is also inductive relative to some m >= n
        z3::expr_vector core(ctx);
        int m = highest_inductive_frame(state->cube, n + 1, level, core);
        // m in [n-1, level]
        assert(static_cast<unsigned>(m + 1) > n);

        if (m >= 0)
        {
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
          result().trace = state;
          return false;
        }
        elapsed = sub_timer.elapsed().count();
        branch = "(finish)";
      }
      log_obligation(branch, level, elapsed);
      elapsed = -1.0;

      // periodically write stats in case of long runs
      if (period >= 100)
      {
        period = 0;
        std::cout << "Stats written" << std::endl;
        SPDLOG_LOGGER_DEBUG(logger.spd_logger, logger.stats.to_string());
        logger.spd_logger->flush();
      }
      else
        period++;
    }

    logger.indent--;
    return true;
  }

  PDResult& PDR::result() { return results.back(); }

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

  void PDR::show_results(std::ostream& out) const
  {
    TextTable t;
    t.setAlignment(0, TextTable::Alignment::RIGHT);
    t.setAlignment(1, TextTable::Alignment::RIGHT);
    t.setAlignment(2, TextTable::Alignment::RIGHT);
    t.setAlignment(3, TextTable::Alignment::RIGHT);

    out << fmt::format("Pebbling strategies for {}:", model.name) << std::endl
        << std::endl;
    out << SEP2 << std::endl;

    std::vector<std::string> header = {"pebbles", "invariant index",
                                       "strategy length", "Total time"};
    t.addRow(header);
    for (const PDResult& res : results)
      t.addRow(res.listing());

    out << t << std::endl << std::endl;

    for (const PDResult& res : results)
    {
      if (res.trace)
      {
        out << fmt::format("Strategy for {} pebbles", res.pebbles_used)
            << std::endl;
        out << fmt::format(
                   "Final: [ {} ]",
                   z3ext::join_expr_vec(model.n_property.currents(), " & "))
            << std::endl
            << std::endl;

        out << "Trace:" << std::endl;
        show_trace(res.trace, out);
        out << SEP << std::endl;
      }
      else
      {
        out << fmt::format("No strategy for {} pebbles",
                           model.get_max_pebbles())
            << std::endl
            << std::endl;
      }
    }
    out << SEP3 << std::endl;
    out << frames_string << std::endl;
    out << SEP2 << std::endl;
    out << solvers_string << std::endl;
  }

  void PDR::store_result()
  {
    std::stringstream ss("Strategy:\n");
    result().pebbles_used = 0;
    result().trace_string = "";

    std::vector<std::tuple<unsigned, std::string, unsigned>> steps;

    std::shared_ptr<State> current = result().trace;
    auto count_pebbled = [](const z3::expr_vector& vec)
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
      int pebbles = count_pebbled(current->cube);
      result().pebbles_used = std::max(result().pebbles_used, pebbles);
      steps.emplace_back(i, z3ext::join_expr_vec(current->cube), pebbles);
      current = current->prev;
    }
	result().trace_length = i+1;
    unsigned i_padding = i / 10 + 1;

    std::string line_form = "{:>{}} |\t [ {} ] No. pebbled = {}";

    std::string initial = z3ext::join_expr_vec(model.get_initial());
    std::string final = z3ext::join_expr_vec(model.n_property.currents());

    ss << fmt::format(line_form, 'I', i_padding, initial, 0) << std::endl;
    for (const auto& [num, vec, count] : steps)
      ss << fmt::format(line_form, num, i_padding, vec, count) << std::endl;
    ss << fmt::format(line_form, 'F', i_padding, final, model.get_f_pebbles())
       << std::endl;

    result().trace_string = ss.str();
  }

  void PDR::show_trace(const std::shared_ptr<State> trace_root, std::ostream& out) const
  {
    std::vector<std::tuple<unsigned, std::string, unsigned>> steps;

    std::shared_ptr<State> current = trace_root;
    auto count_pebbled = [](const z3::expr_vector& vec)
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
                  z3ext::join_expr_vec(model.get_initial()))
        << std::endl;

    for (const auto& [num, vec, count] : steps)
      out << fmt::format("{:>{}} |\t [ {} ] No. pebbled = {}", num, i_padding, vec,
                    count)
          << std::endl;

    out << fmt::format("{:>{}} |\t [ {} ]", 'F', i_padding,
                  z3ext::join_expr_vec(model.n_property.currents()))
        << std::endl;
  }

  Statistics& PDR::stats() { return logger.stats; }

  // LOGGING AND STAT COLLECTION SHORTHANDS
  //
  void PDR::log_and_show(const std::string& str, std::ostream& out)
  {
    out << str << std::endl;
    SPDLOG_LOGGER_INFO(logger.spd_logger, str);
  }
  void PDR::log_iteration()
  {
    std::cout << "###############" << std::endl;
    std::cout << "iterate frame " << k << std::endl;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "");
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| frame {}", logger.tab(), k);
  }

  void PDR::log_cti(const z3::expr_vector& cti)
  {
    (void)cti; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP2);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| cti at frame {}", logger.tab(),
                        k);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(),
                        str::extend::join(cti));
  }

  void PDR::log_propagation(unsigned level, double time)
  {
    std::string msg = fmt::format("Propagation elapsed {}", time);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, msg);
    std::cout << msg << std::endl;
    logger.stats.propagation.add_timed(level, time);
  }

  void PDR::log_top_obligation(size_t queue_size, unsigned top_level,
                               const z3::expr_vector& top)
  {
    (void)queue_size; // ignore unused warning when logging is off
    (void)top_level;  // ignore unused warning when logging is off
    (void)top;        // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| obligations pending: {}",
                        logger.tab(), queue_size);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| top obligation", logger.tab());
    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| {}, [{}]", logger.tab(),
                        top_level, str::extend::join(top));
    logger.indent--;
  }

  void PDR::log_pred(const z3::expr_vector& p)
  {
    (void)p; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| predecessor:", logger.tab());
    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(),
                        str::extend::join(p));
    logger.indent--;
  }

  void PDR::log_state_push(unsigned frame, const z3::expr_vector& p)
  {
    (void)frame; // ignore unused warning when logging is off
    (void)p;     // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| pred is inductive until F_{}",
                        frame - 1, logger.tab());
    SPDLOG_LOGGER_TRACE(logger.spd_logger,
                        "{}| push predecessor to level {}: [{}]", logger.tab(),
                        frame, str::extend::join(p));
  }

  void PDR::log_finish(const z3::expr_vector& s)
  {
    (void)s; // ignore unused warning when logging is off
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| finishing state", logger.tab());
    logger.indent++;
    SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(),
                        str::extend::join(s));
    logger.indent--;
  }

  void PDR::log_obligation(const std::string& type, unsigned l, double time)
  {
    logger.stats.obligations_handled.add_timed(l, time);
    std::string msg = fmt::format("Obligation {} elapsed {}", type, time);
    SPDLOG_LOGGER_TRACE(logger.spd_logger, msg);
    std::cout << msg << std::endl;
  }

} // namespace pdr
