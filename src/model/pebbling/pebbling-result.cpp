#include "pebbling-result.h"
#include "expr.h"
#include "obligation.h"
#include "pebbling-model.h"
#include "result.h"
#include "string-ext.h"
#include "types-ext.h"

#include <cassert>
#include <string>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include <z3++.h>

namespace pdr::pebbling
{
  using mysat::primed::VarVec;
  using std::optional;
  using std::string;
  using std::vector;

  // PebblingResult members
  //
  // Constructors
  IpdrPebblingResult::IpdrPebblingResult(PebblingModel const& m, Tactic t)
      : IpdrResult(m.vars.names(), m.vars.names_p()),
        pebbles_final(m.get_f_pebbles()),
        tactic(t),
        total{ total_time, {}, {} }
  {
    assert(t == Tactic::constrain || t == Tactic::relax ||
           t == Tactic::binary_search);
  }
  IpdrPebblingResult::IpdrPebblingResult(
      VarVec const& vars, unsigned pebbles_final, Tactic t)
      : IpdrResult(vars.names(), vars.names_p()),
        pebbles_final(pebbles_final),
        tactic(t),
        total{ total_time, {}, {} }
  {
  }
  // IpdrResult conversion constructors
  IpdrPebblingResult::IpdrPebblingResult(
      IpdrResult const& r, PebblingModel const& m, Tactic t)
      : IpdrResult(r),
        pebbles_final(m.get_f_pebbles()),
        tactic(t),
        total{ total_time, {}, {} }
  {
    pdr_summaries.resize(0);
    for (const PdrResult& r : original)
    {
      pdr_summaries.push_back(IpdrResult::process_result(r));
    }
  }
  IpdrPebblingResult::IpdrPebblingResult(
      IpdrResult const& r, unsigned pebbles_final, Tactic t)
      : IpdrResult(r),
        pebbles_final(pebbles_final),
        tactic(t),
        total{ total_time, {}, {} }
  {
  }

  IpdrPebblingResult& IpdrPebblingResult::add(
      const PdrResult& r, std::optional<unsigned int> constraint)
  {
    tabulate::Table::Row_t res_row = process_result(r, constraint);
    assert(res_row.size() == summary_header().size() - 1);
    pdr_summaries.push_back(res_row);

    return *this;
  }

  std::string IpdrPebblingResult::end_result() const
  {
    if (total.strategy)
    {
      return fmt::format("Strategy for {} pebbles, with length {}.",
          total.strategy->n_marked, total.strategy->length);
    }

    return "No strategy exists.";
  }

  const IpdrPebblingResult::Data_t& IpdrPebblingResult::get_total() const
  {
    return total;
  }

  const optional<unsigned> IpdrPebblingResult::min_pebbles() const
  {
    if (total.strategy)
      return total.strategy->n_marked;

    return {};
  }

  tabulate::Table::Row_t IpdrPebblingResult::total_row() const
  {
    using fmt::format;
    using std::to_string;

    string time_str = format("{}", total.time);

    string inv_constr, inv_level;
    if (total.inv)
    {
      inv_constr = to_string(total.inv->constraint.value());
      inv_level  = format("F_{}", total.inv->invariant.level);
    }
    else
    {
      assert(tactic == Tactic::constrain &&
             total.strategy->n_marked == pebbles_final);
      // the maximal invariant did not need to be considered and added
      inv_constr = "strategy uses minimal";
      inv_level  = "--";
    }

    string trace_marked, trace_length;
    if (total.strategy)
    {
      trace_marked = to_string(total.strategy->n_marked);
      trace_length = to_string(total.strategy->length);
    }
    else
    {
      // assert(total.inv->constraint == model.n_nodes());
      // never encountered a strategy, no. nodes is the invariant
      trace_marked = "no strategy";
      trace_length = "--";
    }

    return { time_str, inv_constr, inv_level, trace_marked, trace_length };
  }

  // Private Members
  //
  const tabulate::Table::Row_t IpdrPebblingResult::summary_header() const
  {
    auto rv = IpdrResult::summary_header();
    rv.insert(rv.begin(), "pebbled");
    rv.insert(rv.begin(), "constraint");
    return rv;
  }

  const tabulate::Table::Row_t IpdrPebblingResult::total_header() const
  {
    return pebbling_total_header;
  }

  const tabulate::Table::Row_t IpdrPebblingResult::process_result(
      const PdrResult& r, std::optional<unsigned> constraint)
  {
    // row with { invariant level, trace length, time }
    tabulate::Table::Row_t row = IpdrResult::process_result(r);
    // expand to { constraint, marked, invariant level, trace length, time }

    if (r.has_invariant())
    {
      // subsequent constraints are always increasing (relaxed)
      assert(constraint);
      assert(!total.inv || constraint > total.inv->constraint);

      total.inv = { r.invariant(), constraint };
      row.insert(row.begin(), "-");
      std::string inv_str = my::optional::to_string(constraint);
      row.insert(row.begin(), inv_str);

      n_invariants++;
    }
    else
    {
      // subsequent traces are always decreasing in cardinality (constrainec)
      unsigned pebbled = r.trace().n_marked;
      assert(!total.strategy || pebbled < total.strategy->n_marked);

      // tracks the trace with the lowest cardinality
      total.strategy = r.trace();

      row.insert(row.begin(), std::to_string(pebbled));
      row.insert(row.begin(), "-");

      n_traces++;
    }

    switch (tactic)
    {
      case Tactic::constrain:
        // constrain continues until it encounters an invariant
        assert(n_invariants <= 1);
        break;
      case Tactic::relax:
        // relax continues until it encounters a trace
        assert(n_traces <= 1);
        break;
      case Tactic::binary_search: break;
      default: assert(false);
    }

    assert(row.size() == summary_header().size() - 1); // inc time to be added
    return row;
  }

  std::string IpdrPebblingResult::process_trace(const PdrResult& res) const
  {
    return result::trace_table(
        res, vars, vars_p, total.inv->constraint, pebbles_final);
  }

  namespace result
  {
    std::string trace_table(PdrResult const& res,
        std::vector<std::string> vars,
        std::vector<std::string> vars_p,
        PebblingModel const& m)
    {
      return trace_table(
          res, vars, vars_p, m.get_pebble_constraint(), m.get_f_pebbles());
    }

    std::string trace_table(PdrResult const& res,
        std::vector<std::string> vars,
        std::vector<std::string> vars_p,
        std::optional<unsigned> constraint,
        unsigned const f_pebbles)
    {
      using fmt::format;
      using std::to_string;
      using tabulate::Table;
      using TraceState = PdrResult::Trace::TraceState;

      if (res.has_invariant())
      {
        return format("No strategy for constraint {}\n",
            my::optional::to_string(constraint));
      }

      std::sort(vars.begin(), vars.end());
      std::sort(vars_p.begin(), vars_p.end());

      // process trace
      std::stringstream ss;

      Table t;
      // Write top row
      {
        Table::Row_t trace_header = { "", "marked" };
        trace_header.insert(trace_header.end(), vars.begin(), vars.end());
        t.add_row(trace_header);
      }

      auto make_row = [](string a, string b, TraceState const& s,
                          vector<string> const& names)
      {
        vector<string> r = state::marking(s, names);
        r.insert(r.begin(), b);
        r.insert(r.begin(), a);
        Table::Row_t rv;
        rv.assign(r.begin(), r.end());
        return rv;
      };

      // Write strategy states
      {
        unsigned marked = f_pebbles;
        size_t N        = res.trace().length;
        for (size_t i = 0; i < N; i++)
        {
          const TraceState& s = res.trace().states[i];
          unsigned pebbled    = pdr::state::n_marked(s);
          marked              = std::max(marked, pebbled);
          string index_str;
          {
            if (i == 0)
            {
              index_str = "I";
            }
            else if (i == N - 1)
              index_str = "(!P) " + to_string(i);
            else
              index_str = to_string(i);
          }

          Table::Row_t row_marking = make_row(
              index_str, to_string(pebbled), s, (i < N - 1 ? vars : vars_p));
          t.add_row(row_marking);
        }
        ss << format("Strategy for {} pebbles", marked) << std::endl
           << std::endl;
      }

      t.format().font_align(tabulate::FontAlign::right);

      ss << tabulate::MarkdownExporter().dump(t);
      return ss.str();
    }
  } // namespace result
} // namespace pdr::pebbling
