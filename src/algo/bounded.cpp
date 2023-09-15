#include "bounded.h"
#include "cli-parse.h"
#include "dag.h"
#include "io.h"
#include "pebbling-result.h"
#include "result.h"
#include "tactic.h"
#include "z3-ext.h"
#include <algorithm>
#include <cassert>
#include <regex>
#include <tabulate/table.hpp>
#include <z3++.h>

namespace bounded
{
  using std::string;
  using std::string_view;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  BoundedPebbling::BoundedPebbling(
      const dag::Graph& G, my::cli::ArgumentList const& args)
      : context(),
        graph(G),
        solver(context),
        lit_names(),
        n_lits(G.nodes.size())
  {
    using namespace my::io;

    context.set("timeout", time_limit * 1000);
    context.set("model", true);
    solver.set("sat.cardinality.solver", true);
    solver.set("cardinality.solver", true);
    // solver.set("sat.random_seed", ctx.seed);
    for (string_view s : G.nodes)
      lit_names.emplace_back(s);
    std::sort(lit_names.begin(), lit_names.end());

    result_out = args.folders.file_in_run("trace");
  }

  void BoundedPebbling::reset()
  {
    solver.reset();
    current_bound = {};
    cardinality   = {};
  }

  expr BoundedPebbling::lit(std::string_view name, size_t time_step)
  {
    std::string full_name = fmt::format("{}.{}", name, time_step);
    return context.bool_const(full_name.c_str());
  }

  expr BoundedPebbling::constraint(const expr_vector& lits)
  {
    return z3::atmost(lits, cardinality.value());
  }

  ConstrainedExpr BoundedPebbling::initial()
  {
    expr_vector cube(context), lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, 0);
      lits.push_back(l);
      cube.push_back(!l);
    }

    return { z3::mk_and(cube), constraint(lits) };
  }

  ConstrainedExpr BoundedPebbling::final()
  {
    expr_vector cube(context), lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, current_bound.value());
      lits.push_back(l);
      if (graph.is_output(n))
        cube.push_back(l);
      else
        cube.push_back(!l);
    }

    return { z3::mk_and(cube), constraint(lits) };
  }

  ConstrainedExpr BoundedPebbling::trans_step(size_t i)
  {
    expr_vector next(context), T(context);
    for (std::string_view node : lit_names) // every node has a transition
    {
      expr source   = lit(node, i);
      expr source_p = lit(node, i + 1);
      next.push_back(source_p);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (std::string_view child : graph.get_children(node))
      {
        // clang-format off
        T.push_back( source || !source_p || lit(child, i));
        T.push_back(!source ||  source_p || lit(child, i));
        T.push_back( source || !source_p || lit(child, i+1));
        T.push_back(!source ||  source_p || lit(child, i+1));
        // clang-format on
      }
    }

    return { z3::mk_and(T), constraint(next) };
  }

  z3::check_result BoundedPebbling::check(size_t steps, double allowance)
  {
    push_transitions(steps);
    context.set("timeout", (int)(allowance * 1000.0));
    z3::check_result result = solver.check(final());
    // assert(result != z3::check_result::unknown);

    return result;
  }

  void BoundedPebbling::push_transitions(size_t steps)
  {
    assert(steps > 0);
    if (!current_bound)
    {
      ConstrainedExpr I = initial();
      solver.add(I.expression);
      solver.add(I.constraint);
      current_bound = 0;
    }

    for (size_t& i = current_bound.value(); i < steps; i++)
      solver.add(trans_step(i));
  }

  pdr::pebbling::IpdrPebblingResult BoundedPebbling::run()
  {
    using fmt::format;
    using std::endl;

    std::vector<std::string> names(graph.nodes.cbegin(), graph.nodes.cend());
    pdr::pebbling::IpdrPebblingResult total(
        names, names, graph.output.size(), pdr::Tactic::constrain);

    size_t pebbles = graph.nodes.size();
    bool done  = false;
    trace      = {};
    total_time = 0.0;
    sub_times.resize(0);
    timer.reset();

    std::cout << "start BMC" << std::endl;

    // constrain down to optimum
    while (!done && pebbles >= graph.output.size())
    {
      card_timer.reset();

      auto elapsed = [this]() { return card_timer.elapsed().count(); };

      cardinality = pebbles;

      //
      for (size_t length = 1;; length++)
      {
        step_timer.reset();

        z3::check_result r = check(length, dtime_limit - elapsed());

        // double dt = step_timer.elapsed().count();
        // std::cout << fmt::format("\t{:>4}: {}", length, dt) << std::endl;
        // sub_times.push_back(dt);

        if (r == z3::check_result::sat)
        {
          // std::cout << "FOUND at " << cardinality.value() << std::endl;
          auto r = pdr::PdrResult::found_trace(get_trace(length))
                       .with_duration(elapsed());
          total.add(r, cardinality);
          reset();
          break;
        }

        if (r == z3::check_result::unknown || elapsed() >= dtime_limit)
        {
          auto r = pdr::PdrResult::empty_true().with_duration(elapsed());
          total.add(r, cardinality);
          done = true;
          break;
        }
      }
      if (total.get_total().strategy)
        pebbles = total.get_total().strategy->n_marked - 1;
      else
        pebbles--;
    }

    total_time = timer.elapsed().count();

    std::cout << "done" << std::endl;

    dump_times(result_out);

    return total;
  }

  // get the name, mark and timestep of m[i]
  Marking get_time_step(const z3::model& m, size_t i)
  {
    assert(i < m.num_consts());
    Marking rv;
    {
      // matches the [name].[timestep] format
      std::regex postfix(R"(^([[:alnum:]_]+).([[:digit:]]+)$)");
      std::smatch postfix_match;
      std::string lit_str{ m[i]().to_string() };
      if (!std::regex_match(lit_str, postfix_match, postfix))
      {
        assert(false);
      }
      assert(postfix_match.size() == 3);

      rv.name            = postfix_match[1];
      string postfix_str = postfix_match[2];
      sscanf(postfix_str.c_str(), "%zu", &rv.timestep);
    }
    {
      z3::expr assignment = m.get_const_interp(m[i]);
      assert(assignment.is_bool());
      rv.mark = assignment.is_true();
    }

    return rv;
  }

  std::string BoundedPebbling::strategy_table(
      const vector<TraceRow>& content) const
  {
    using std::to_string;
    using tabulate::Table;
    Table header, t;
    {
      Table::Row_t header{ "", "marked" };
      header.insert(header.end(), lit_names.begin(), lit_names.end());
      t.add_row(header);
    }
    for (size_t i = 0; i < content.size(); i++)
    {
      const TraceRow& tr = content[i];
      Table::Row_t row({ to_string(i), to_string(tr.marked) });
      row.insert(row.end(), tr.begin(), tr.end());
      t.add_row(row);
    }

    t.format().hide_border_top().hide_border_bottom();

    std::stringstream ss;
    ss << t;
    return ss.str();
  }

  BoundedPebbling::TraceVec BoundedPebbling::get_trace(size_t length) const
  {
    TraceVec rv;

    z3::model witness = solver.get_model();
    rv.clear();
    rv.resize(length + 1);

    for (size_t i = 0; i < witness.size(); i++)
    {
      Marking lit = get_time_step(witness, i);
      assert(lit.timestep < length + 1);
      rv.at(lit.timestep).push_back(z3ext::LitStr(lit.name, lit.mark));
    }

    for (TraceState& s : rv)
    {
      std::sort(s.begin(), s.end(),
          [](z3ext::LitStr const& a, z3ext::LitStr const& b)
          { return a.name < b.name; });
    }

    return rv;
  }

  // void BoundedPebbling::store_strategy(size_t length)
  // {
  //   auto l_begin = lit_names.begin();
  //   auto l_end   = lit_names.end();

  //   z3::model witness = solver.get_model();
  //   trace->clear();
  //   trace->reserve(length);

  //   string_view longest_lit = *std::max_element(l_begin, l_end,
  //       [](string_view a, string_view b) { return a.size() < b.size(); });

  //   for (size_t i = 0; i < witness.size(); i++)
  //   {
  //     Marking lit = get_time_step(witness, i);

  //     // fill trace to accomodate literal
  //     while (trace->size() < lit.timestep + 1)
  //       trace->emplace_back(lit_names.size(), "?");

  //     // find and fill lit entries with "XXX" or "   "
  //     auto it = std::lower_bound(l_begin, l_end, lit.name);
  //     if (it != lit_names.end() && *it == lit.name) // it points to s
  //     {
  //       string fill_X = fmt::format("{:X^{}}", "", longest_lit.size());
  //       size_t index  = it - l_begin;
  //       trace->at(lit.timestep).mark(index, lit, fill_X);
  //     }
  //   }
  // }

  void BoundedPebbling::dump_times(std::ostream& out) const
  {
    out << fmt::format("Total time: {}", total_time) << std::endl << std::endl;

    // for (double t : sub_times)
    // std::cout << fmt::format("{}", t) << std::endl;
  }
} // namespace bounded
