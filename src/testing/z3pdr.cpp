#include "z3pdr.h"
#include "dag.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "result.h"
#include "string-ext.h"
#include "vpdr.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iterator>
#include <numeric>
#include <optional>
#include <regex>
#include <spdlog/stopwatch.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tabulate/table.hpp>
#include <z3++.h>
#include <z3_api.h>
#include <z3_fixedpoint.h>
#include <z3_spacer.h>

namespace pdr::test
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::sort_vector;

  // done in setup, defines relation between state and next state
  // if step(state, state.p) |-> true && state, then state.p
  z3PDR::z3PDR(Context c, Logger& l, IModel& m)
      : vPDR(c, l, m) 
  {
  }

  z3::fixedpoint z3PDR::mk_prepare_fixedpoint()
  {
    z3::fixedpoint engine(ctx);
    z3::params p(ctx);
    p.set("engine", "spacer"); // z3 pdr implementation
    p.set("spacer.random_seed", ctx.seed);
    p.set("spacer.push_pob", true); // pushing blocked facts
    // p.set("spacer.trace_file", "spacer-trace.txt");

    engine.set(p);
    // z3::set_param("trace", true);

    return engine;
  }

  void z3PDR::reset() { last_result = z3::check_result::unknown; }

  std::optional<size_t> z3PDR::constrain()
  {
    last_result = z3::check_result::unknown;
    ctx.type    = Tactic::constrain;
    MYLOG_DEBUG(logger, "constraining ipdr not supported for spacer");
    return {};
  }

  void z3PDR::relax()
  {
    last_result = z3::check_result::unknown;
    ctx.type    = Tactic::relax;
    MYLOG_DEBUG(logger, "relaxing ipdr not supported for spacer");
  }

  PdrResult z3PDR::run()
  {
    using std::cout;
    using std::endl;

    log_start();
    timer.reset();

    z3::fixedpoint engine = mk_prepare_fixedpoint();
    ts.load_initial(engine);
    ts.load_transition(engine);
    expr target = ts.create_fp_target();
    // MYLOG_INFO(logger, "Initial State:\n{}", ts.fp_I->expr.to_string());
    // MYLOG_INFO(logger, "Transition System:\n{}", T.expr.to_string());
    // MYLOG_INFO(logger, "Target:\n{}", target.to_string());
    z3::func_decl target_decl = ts.state;

    MYLOG_DEBUG(logger, "Fixedpoint engine:\n{}", engine.to_string());

    spdlog::stopwatch timer;
    last_result = engine.query(target);
    double time = timer.elapsed().count();

    // cover_string = "fixedpoint delta-covers:\n";
    // {
    //   int n_levels = engine.get_num_levels(target_decl);
    //   assert(n_levels > 0);
    //   for (int i{ -1 }; i < n_levels; i++)
    //   {
    //     cover_string += fmt::format("-- level {}\n", i);
    //     cover_string += fmt::format(
    //         "{}\n", engine.get_cover_delta(i, target_decl).to_string());
    //     cover_string += "\n";
    //   }
    // }
    // MYLOG_DEBUG(logger, cover_string);

    vector<std::string> trace;
    unsigned n_levels;
    optional<PdrResult> rv;
    switch (last_result)
    {
      case z3::check_result::sat:
        trace = get_trace(engine);
        MYLOG_DEBUG(logger, "SAT fixedpoint");
        MYLOG_DEBUG(logger, "Trace: {}", trace);
        rv = PdrResult::found_trace(ts.fp_trace_states(engine))
                 .with_duration(time);
        break;
      case z3::check_result::unsat:
        // n_levels = engine.get_num_levels(target_decl);
        MYLOG_DEBUG(logger, "UNSAT fixedpoint");
        MYLOG_DEBUG(logger, "Invariant found after {} levels", n_levels);
        rv = PdrResult::found_invariant(n_levels).with_duration(time);
        break;
      default:
        MYLOG_WARN(logger, "Undefined fixedpoint result");
        rv = PdrResult::empty_true();
        break;
    }
    double final_time = timer.elapsed().count();
    log_pdr_finish(rv.value(), final_time);
    rv->time = final_time;
    return rv.value();
  }

  void z3PDR::show_solver(std::ostream& out) const
  {
    out << "no solver, fixed point engine covers:" << std::endl
        << cover_string << std::endl;
  }

  vector<std::string> z3PDR::get_trace(z3::fixedpoint& engine)
  {
    assert(last_result == z3::check_result::sat);

    z3::symbol raw(
        ctx(), Z3_fixedpoint_get_rule_names_along_trace(ctx(), engine));
    MYLOG_TRACE(logger, "TRACE: {}", raw.str());
    vector<std::string> trace = str::ext::split(raw.str(), ';');
    trace.erase(std::remove_if(trace.begin(), trace.end(),
                    [](string_view a) { return a == "->"; }),
        trace.end());

    std::reverse(trace.begin(), trace.end());
    // assert(trace.at(0) == "I");

    return trace;
  }

  std::vector<z3::expr> extract_trace_states(z3::fixedpoint& engine)
  {
    std::vector<z3::expr> rv;
    // answer {
    //  arg(0):
    //  arg(1):
    //  arg(2): destination
    // }
    expr answer = engine.get_answer().arg(0).arg(1);

    while (answer.num_args() == 3)
    {
      assert(answer.arg(2).get_sort().is_bool());
      rv.push_back(answer.arg(2));

      answer = answer.arg(1);
    }

    assert(answer.num_args() == 2);
    rv.push_back(answer.arg(1));
    std::reverse(rv.begin(), rv.end());
    return rv;
  }

  PdrResult::Trace::TraceVec z3PDR::get_trace_states(z3::fixedpoint& engine)
  {
    using tabulate::Table;
    using z3ext::LitStr;
    using namespace str::ext;
    assert(last_result == z3::check_result::sat);

    vector<vector<LitStr>> rv;
    const vector<string> header = ts.vars.names();
    auto answer                 = engine.get_answer().arg(0).arg(1);
    MYLOG_DEBUG(logger, "Raw answer:");
    for (size_t i{ 0 }; i < answer.num_args(); i++)
    {
      MYLOG_DEBUG(logger, "{} ------{}------\n{}", i,
          answer.arg(i).get_sort().to_string(), answer.arg(i).to_string());
    }

    vector<expr> states = extract_trace_states(engine);

    auto invalid = [](std::string_view s)
    {
      return std::invalid_argument(
          fmt::format("\"{}\" is not a valid state in the trace", s));
    };

    // use regex to extract the assignments of "true" and "false" from a state
    // they are in order of ts.vars.names()
    const std::regex marking(R"(\(state((?:\s+(?:true|false))*)\))");
    std::smatch match;

    MYLOG_DEBUG(logger, "Trace markings:");
    for (size_t i{ 0 }; i < states.size(); i++)
    {
      std::string state_str(states[i].to_string());
      if (!std::regex_match(state_str, match, marking))
        throw invalid(state_str);
      assert(match.size() == 2);

      string mark_string = match[1];
      trim(mark_string); // remove white spaces at beginning and end
      // replace all white spaces by " "
      mark_string = std::regex_replace(mark_string, std::regex("\\s+"), " ");

      vector<string> marks = split(mark_string, ' ');

      assert(marks.size() == header.size());
      vector<LitStr> state;
      for (size_t j{ 0 }; j < marks.size(); j++)
      {
        if (marks[j] == "true")
          state.emplace_back(header.at(j), true);
        else if (marks[j] == "false")
          state.emplace_back(header.at(j), false);
        else
          throw invalid(state_str);
      }
      rv.push_back(state);
      MYLOG_DEBUG(logger, "- {}",
          [&state]() -> string
          {
            string rv{ "" };
            for (LitStr const& l : state)
              rv += l.to_string() + ", ";
            return rv;
          }());
    }

    return rv;
  }
} // namespace pdr::test
