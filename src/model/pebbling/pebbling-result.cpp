#include "pebbling-result.h"
#include "obligation.h"
#include "string-ext.h"

#include <cassert>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <z3++.h>

namespace pdr::pebbling
{
  using std::string;
  using std::vector;

  // PebblingResult members
  //
  PebblingResult::PebblingResult(const PebblingModel& m, Tactic t)
      : IpdrResult(m), model(m), tactic(t), total{ total_time, {}, {} }
  {
    assert(t == Tactic::constrain || t == Tactic::relax);
  }

  PebblingResult::PebblingResult(
      const IpdrResult& r, const PebblingModel& m, Tactic t)
      : IpdrResult(r), model(m), tactic(t), total{ total_time, {}, {} }
  {
    rows.resize(0);
    for (const PdrResult& r : original)
    {
      tabulate::Table::Row_t res_row = table_row(r);
      assert(res_row.size() == header().size());
      rows.push_back(res_row);
    }
  }

  void PebblingResult::add_summary_to(tabulate::Table& t) const
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

    t.add_row({ time_str, inv_constr, inv_level, trace_marked, trace_length });
  }

  void PebblingResult::show_raw(std::ostream& out) const
  {
    IpdrResult::show(out);
  }

  void PebblingResult::show(std::ostream& out) const
  {
    using fmt::format;
    using std::to_string;

    tabulate::Table table;
    table.format().font_align(tabulate::FontAlign::right);

    table.add_row(result::summary_header);
    add_summary_to(table);

    out << table << std::endl;
    auto latex = tabulate::LatexExporter().dump(table);
    out << latex << std::endl;
  }

  const PebblingResult::Data_t& PebblingResult::get_total() const
  {
    return total;
  }

  // if decreasing: min strategy = the latest of the multiple strategies
  // if increasing: min strategy = the only (first) strategy found
  const std::optional<unsigned> PebblingResult::min_pebbles() const
  {
    if (total.strategy)
      return total.strategy->pebbled;

    return {};
  }

  // Private Members
  //
  const tabulate::Table::Row_t PebblingResult::header() const
  {
    return result::result_header;
  }

  const tabulate::Table::Row_t PebblingResult::table_row(const PdrResult& r)
  {
    // row with { invariant level, trace length, time }
    tabulate::Table::Row_t row = IpdrResult::table_row(r);
    // expand to { constraint, marked, invariant level, trace length, time }

    if (r.has_invariant()) // only happens multiple times in increasing
    {
      // when done multiple time, we are increasing the constraint
      optional<unsigned> constraint = model.get_max_pebbles();
      assert(constraint);
      assert(!total.inv || constraint > total.inv->constraint);

      total.inv = { r.invariant(), constraint };

      row.insert(row.begin(), "-");
      std::string inv_str = constraint ? std::to_string(*constraint) : "none";
      row.insert(row.begin(), inv_str);

      if (tactic == Tactic::constrain)
      {
        assert(++n_invariants <= 1);
      }
    }
    else // only happens multiple times in decreasing
    {
      // when done multiple time, we are decreasing the constraint,
      // decreasing the pebbles used
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
      if (model.get_max_pebbles())
        return format("No strategy for {}\n", *model.get_max_pebbles());
      else
        return "No strategy\n";
    }

    // process trace
    std::stringstream ss;
    std::vector<std::string> lits  = model.vars.names();
    std::vector<std::string> litsp = model.vars.names_p();
    std::sort(lits.begin(), lits.end());

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
      std::vector<std::string> r = state::marking(s, names, longest);
      r.insert(r.begin(), b);
      r.insert(r.begin(), a);
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write strategy states
    {
      unsigned marked = model.get_f_pebbles();
      size_t N        = res.trace().states.size();
      for (size_t i = 0; i < N; i++)
      {
        const z3::expr_vector& s = res.trace().states[i];
        unsigned pebbled         = state::no_marked(s);
        marked                   = std::max(marked, pebbled);
        string index_str         = (i == 0) ? "I" : to_string(i);
        assert(i > 0 || z3ext::eq(s, model.get_initial()));

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
