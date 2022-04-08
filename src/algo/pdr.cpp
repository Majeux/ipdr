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
    make_result(rv);

    logger.stats.elapsed = final_time;
    logger.stats.write(constraint_str());
    logger.stats.write();
    logger.stats.clear();
    store_frame_strings();
    if (!rv)
      shortest_strategy = rv.marked;
    logger.indent = 0;

    return rv;
  }

  // returns true if the model survives initiation
  Result PDR::init()
  {
    logger.and_whisper("Start initiation");
    assert(frames.frontier() == 0);

    const PebblingModel& m = ctx.c_model();
    expr_vector notP       = m.n_property.currents();

    if (frames.init_solver.check(notP))
    {
      logger.whisper("I =/> P");
      return Result::found_trace(m.get_initial());
    }

    expr_vector notP_next = m.n_property.nexts();
    if (frames.SAT(0, notP_next))
    { // there is a transitions from I to !P
      logger.show("I & T =/> P'");
      expr_vector bad_cube = frames.get_solver(0).witness_current();
      return Result::found_trace(bad_cube);
    }

    frames.extend();

    logger.and_whisper("Survived Initiation");
    return Result::empty_true();
  }

  Result PDR::iterate()
  {
    // I => P and I & T ⇒ P' (from init)
    expr_vector notP_next = ctx.c_model().n_property.nexts();
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

        Result res = block(*cti, k - 1);
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
      frames.log_solvers();

      if (invariant_level >= 0)
        return Result::found_invariant(invariant_level);
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
      sub_timer.reset();
      double elapsed;
      string branch;

      auto [n, state, depth] = *(obligations.begin());
      assert(n <= k);
      log_top_obligation(obligations.size(), n, state->cube);

      // !state -> !state
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
        if (m >= 0)
        {
          expr_vector smaller_pred = generalize(core, m);
          frames.remove_state(smaller_pred, m + 1);

          if (static_cast<unsigned>(m + 1) <= k)
          {
            log_state_push(m + 1);
            obligations.emplace(m + 1, pred, depth + 1);
          }
        }
        else // intersects with I
          return Result::found_trace(pred);

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

        if (m >= 0)
        {
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
        }
        else
          return Result::found_trace(state);

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
    return Result();
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
       << frames.solvers_str() << endl;

    logger.stats.solver_dumps.push_back(ss.str());
  }

  void PDR::show_solver(std::ostream& out) const // TODO
  {
    for (const std::string& s : logger.stats.solver_dumps)
      out << s << std::endl << std::endl;
  }

  void PDR::make_result(Result& result)
  {
    using string_vec = std::vector<std::string>;
    using std::string_view;
    const PebblingModel& model = ctx.c_model();

    if (result)
    {
      if (frames.max_pebbles)
        result.trace_string =
            format("No strategy for {}\n", *frames.max_pebbles);
      else
        result.trace_string = "No strategy\n";
      return;
    }

    string_vec lits;
    {
      expr_vector z3_lits = ctx.c_model().lits.currents();
      std::transform(z3_lits.begin(), z3_lits.end(), std::back_inserter(lits),
          [](expr l) { return l.to_string(); });
      std::sort(lits.begin(), lits.end());
    }

    auto str_size_cmp = [](string_view a, string_view b)
    { return a.size() < b.size(); };
    size_t largest =
        std::max_element(lits.begin(), lits.end(), str_size_cmp)->size();

    TextTable t('|');
    {
      string_vec header = { "", "" };
      header.insert(header.end(), lits.begin(), lits.end());
      t.addRow(header);
    }

    auto row = [&lits, largest](string a, string b, const State& s)
    {
      string_vec rv = s.marking(lits, largest);
      rv.insert(rv.begin(), b);
      rv.insert(rv.begin(), a);
      return rv;
    };

    // Write initial state
    {
      expr_vector initial_state = model.get_initial();
      string_vec initial_row =
          row("I", "No. pebbled = 0", State(initial_state));
      t.addRow(initial_row);
    }

    // Write strategy states
    {
      unsigned i = 0;
      for (const State& s : result)
      {
        i++;
        unsigned pebbles = s.no_marked();
        result.marked    = std::max(result.marked, pebbles);
        string_vec row_marking =
            row(std::to_string(i), format("No. pebbled = {}", pebbles), s);
        t.addRow(row_marking);
      }
      result.trace_length = i + 1;
    }

    // Write final state
    {
      expr_vector final_state = model.n_property.currents();
      string_vec final_row =
          row("F", format("No. pebbled = {}", model.get_f_pebbles()),
              State(final_state));
      t.addRow(final_row);
    }

    for (unsigned i = 0; i < t.rows()[0].size(); i++)
      t.setAlignment(i, TextTable::Alignment::RIGHT);

    std::stringstream ss;
    ss << "Strategy for " << result.marked << " pebbles" << std::endl;
    ss << t;
    result.trace_string = ss.str();

    result.clean_trace(); // information is stored in string, deallocate trace
  }

  int PDR::length_shortest_strategy() const { return shortest_strategy; }

} // namespace pdr
