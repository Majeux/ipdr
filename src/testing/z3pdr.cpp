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
#include <dbg.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iterator>
#include <numeric>
#include <spdlog/stopwatch.h>
#include <sstream>
#include <string>
#include <z3++.h>
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
  z3PDR::z3PDR(Context c, Logger& l, Z3Model& m) : vPDR(c, l), ts(m) {}

  z3::fixedpoint z3PDR::mk_prepare_fixedpoint()
  {
    z3::fixedpoint engine(ctx);
    z3::params p(ctx);
    p.set("engine", "spacer");      // z3 pdr implementation
    p.set("spacer.push_pob", true); // pushing blocked facts
    p.set("print_answer", true);
    // p.set("spacer.p3.share_invariants", true); // invariant lemmas
    // p.set("spacer.p3.share_lemmas", true);     // frame lemmas (clauses?)
    engine.set(p);
    // std::cout << "Settings" << std::endl
    //           << engine.get_param_descrs() << std::endl
    //           << std::endl;
    // std::cout << "z3::fixedpoint options" << std::endl
    //           << str::ext::indent(engine.help()) << std::endl
    //           << std::endl;
    return engine;
  }

  void z3PDR::reset() { last_result = z3::check_result::unknown; }

  PdrResult z3PDR::run()
  {
    using std::cout;
    using std::endl;

    z3::fixedpoint engine = mk_prepare_fixedpoint();
    // dbg(engine.help());

    ts.add_initial(engine);
    ts.add_transitions(engine);
    // std::cout << ts.to_string() << std::endl;

    z3::func_decl t_decl = ts.get_target().decl();
    cout << endl << "Target" << endl;
    cout << ts.get_target().to_string() << endl << endl;

    cout << "Fixedpoint engine" << endl << engine.to_string() << endl;

    spdlog::stopwatch timer;
    last_result = ts.reach_target(engine);
    double time = timer.elapsed().count();

    std::vector<std::string> trace;
    switch (last_result)
    {
      case z3::check_result::sat:
        trace = get_trace(engine);
        cout << "sat fixed point " << fmt::format("trace: {}", trace) << endl;
        // get_trace_states(engine);
        return PdrResult::incomplete_trace(trace.size() - 1)
            .with_duration(time);
      case z3::check_result::unsat:
        cout << "unsat fixed point";
        return PdrResult::found_invariant(engine.get_num_levels(t_decl))
            .with_duration(time);
      default: break;
    }

    std::cerr << "unknown result" << endl;
    return PdrResult::empty_true();
  }

  void z3PDR::show_solver(std::ostream& out) const
  {
    out << "no solver, fixed point engine" << std::endl;
    // out << engine.to_string() << std::endl;
  }

  std::vector<std::string> z3PDR::get_trace(z3::fixedpoint& engine)
  {
    assert(last_result == z3::check_result::last_result);

    z3::symbol raw(
        ctx(), Z3_fixedpoint_get_rule_names_along_trace(ctx(), engine));
    std::vector<std::string> trace = str::ext::split(raw.str(), ';');
    trace.erase(std::remove_if(trace.begin(), trace.end(),
                    [](string_view a) { return a == "->"; }),
        trace.end());

    std::reverse(trace.begin(), trace.end());
    assert(trace.at(0) == "I");

    return trace;
  }

  std::string expr_info(expr const& e, unsigned level = 0)
  {
    std::string indent(level, '\t');
    std::stringstream ss;

    std::cout << indent << "type:" << e.get_sort() << std::endl;
    std::cout << indent << "args:" << e.num_args();

    return ss.str();
  }

  std::vector<std::string> z3PDR::get_trace_states(z3::fixedpoint& engine)
  {
    assert(last_result == z3::check_result::last_result);

    expr answer = engine.get_answer();
    // first argument is proof
    // second is query
    // third is result

    std::cout << "--\n--raw answer" << answer << std::endl << std::endl;

    // std::cout << expr_info(answer, 0) << std::endl;
    // for (size_t i{ 0 }; i < answer.num_args(); i++)
    // {
    //   std::cout << "\t==" << answer.arg(i) << std::endl
    //             << expr_info(answer.arg(i), 1) << std::endl;
    // }

    // std::cout << "L2" << std::endl;
    // expr proof = answer.arg(0);
    // for (size_t i{ 0 }; i < proof.num_args(); i++)
    // {
    //   std::cout << "\t==\n"
    //             << expr_info(proof.arg(i), 2) << std::endl
    //             << "\t" << proof.arg(i) << std::endl;
    // }

    return {};
  }
} // namespace pdr::test
