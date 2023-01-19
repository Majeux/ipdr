#include "pebbling-result.h"
#include "expr.h"
#include "obligation.h"
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
  PebblingResult::PebblingResult(PebblingModel const& m, Tactic t)
      : IpdrResult(m),
        pebbles_final(m.get_f_pebbles()),
        pebble_constraint(m.get_pebble_constraint()),
        tactic(t),
        total{ total_time, {}, {} }
  {
    assert(t == Tactic::constrain || t == Tactic::relax);
  }
  PebblingResult::PebblingResult(z3::expr_vector initial, VarVec const& vars,
      unsigned pebbles_final, optional<unsigned> pebbles_max, Tactic t)
      : IpdrResult(initial, vars),
        pebbles_final(pebbles_final),
        pebble_constraint(pebbles_max),
        tactic(t),
        total{ total_time, {}, {} }
  {
  }
  // IpdrResult conversion constructors
  PebblingResult::PebblingResult(
      IpdrResult const& r, PebblingModel const& m, Tactic t)
      : IpdrResult(r),
        pebbles_final(m.get_f_pebbles()),
        pebble_constraint(m.get_pebble_constraint()),
        tactic(t),
        total{ total_time, {}, {} }
  {
    pdr_summaries.resize(0);
    for (const PdrResult& r : original)
    {
      tabulate::Table::Row_t res_row = process_row(r);
      pdr_summaries.push_back(res_row);
    }
  }
  PebblingResult::PebblingResult(IpdrResult const& r, unsigned pebbles_final,
      optional<unsigned> pebbles_max, Tactic t)
      : IpdrResult(r),
        pebbles_final(pebbles_final),
        pebble_constraint(pebbles_max),
        tactic(t),
        total{ total_time, {}, {} }
  {
  }

  std::string PebblingResult::end_result() const
  {
    if (total.strategy)
    {
      return fmt::format("Strategy for {} pebbles, with length {}.",
          total.strategy->pebbled, total.strategy->trace.length);
    }

    return "No strategy exists.";
  }

  const PebblingResult::Data_t& PebblingResult::get_total() const
  {
    return total;
  }

  const optional<unsigned> PebblingResult::min_pebbles() const
  {
    if (total.strategy)
      return total.strategy->pebbled;

    return {};
  }

  tabulate::Table::Row_t PebblingResult::total_row() const
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
             total.strategy->pebbled == model.get_f_pebbles());
      // the maximal invariant did not need to be considered and added
      inv_constr = "strategy uses minimal";
      inv_level  = "--";
    }

    string trace_marked, trace_length;
    if (total.strategy)
    {
      trace_marked = to_string(total.strategy->pebbled);
      trace_length = to_string(total.strategy->trace.length);
    }
    else
    {
      assert(total.inv->constraint == model.n_nodes());
      // never encountered a strategy, no. nodes is the invariant
      trace_marked = "no strategy";
      trace_length = "--";
    }

    return { time_str, inv_constr, inv_level, trace_marked, trace_length };
  }

  // Private Members
  //
  const tabulate::Table::Row_t PebblingResult::summary_header() const
  {
    return pebbling_summary_header;
  }

  const tabulate::Table::Row_t PebblingResult::total_header() const
  {
    return pebbling_total_header;
  }

  const tabulate::Table::Row_t PebblingResult::process_row(const PdrResult& r)
  {
    // row with { invariant level, trace length, time }
    tabulate::Table::Row_t row = IpdrResult::process_row(r);
    // expand to { constraint, marked, invariant level, trace length, time }

    if (r.has_invariant()) // only happens multiple times in increasing
    {
      // when done multiple time, we are increasing the constraint
      assert(pebble_constraint);
      assert(!total.inv || constraint > total.inv->constraint);

      total.inv = { r.invariant(), pebble_constraint };
      row.insert(row.begin(), "-");
      std::string inv_str = my::optional::to_string(pebble_constraint);
      row.insert(row.begin(), inv_str);

      if (tactic == Tactic::constrain)
      {
        assert(++n_invariants <= 1);
      }
    }
    else // only happens multiple times in decreasing
    {
      // when done multiple time, we are decreasing the constraint,
      // track the strategy with the lowest number of pebbles
      unsigned pebbled = r.trace().total_pebbled();
      assert(!total.strategy || pebbled < total.strategy->pebbled);

      total.strategy = { r.trace(), pebbled };

      row.insert(row.begin(), std::to_string(pebbled));
      row.insert(row.begin(), "-");

      if (tactic == Tactic::relax)
      {
        assert(++n_traces <= 1);
      }
    }

    assert(row.size() == summary_header().size());
    return row;
  }

  std::string PebblingResult::process_trace(const PdrResult& res) const
  {
    using fmt::format;
    using std::string;
    using std::string_view;
    using std::to_string;
    using tabulate::Table;
    using z3::expr;
    using z3::expr_vector;

    if (res.has_invariant())
    {
      return format("No strategy for constraint {}\n",
          my::optional::to_string(pebble_constraint));
    }

    // process trace
    std::stringstream ss;
    vector<string> lits = vars.names(), litsp = vars.names_p();
    std::sort(lits.begin(), lits.end());
    std::sort(litsp.begin(), litsp.end());

    size_t longest =
        std::max_element(lits.begin(), lits.end(), str::ext::size_lt)->size();

    Table t;
    // Write top row
    {
      Table::Row_t trace_header = { "", "marked" };
      trace_header.insert(trace_header.end(), lits.begin(), lits.end());
      t.add_row(trace_header);
    }

    auto make_row = [&longest](string a, string b, const expr_vector& s,
                        const vector<string>& names)
    {
      vector<string> r = state::marking(s, names, longest);
      r.insert(r.begin(), b);
      r.insert(r.begin(), a);
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write strategy states
    {
      unsigned marked = pebbles_final;
      size_t N        = res.trace().states.size();
      for (size_t i = 0; i < N; i++)
      {
        const expr_vector& s = res.trace().states[i];
        unsigned pebbled     = state::no_marked(s);
        marked               = std::max(marked, pebbled);
        string index_str;
        {
          if (i == 0)
          {
            assert(z3ext::quick_implies(s, model.get_initial())); // s \in I
            index_str = "I";
          }
          else if (i == N - 1)
            index_str = "(!P) " + to_string(i);
          else
            index_str = to_string(i);
        }

        Table::Row_t row_marking = make_row(
            index_str, to_string(pebbled), s, (i < N - 1 ? lits : litsp));
        t.add_row(row_marking);
      }
      ss << format("Strategy for {} pebbles", marked) << std::endl << std::endl;
    }

    t.format().font_align(tabulate::FontAlign::right);

    ss << tabulate::MarkdownExporter().dump(t);
    return ss.str();
  }
} // namespace pdr::pebbling
