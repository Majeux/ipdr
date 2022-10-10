#include "pebbling-result.h"
#include "obligation.h"

#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>

namespace pdr::pebbling
{
  using std::string;
  using std::vector;

  namespace // helper
  {
    bool str_size_cmp(string_view a, string_view b)
    {
      return a.size() < b.size();
    };
  } // namespace

  // PebblingResult members
  //
  PebblingResult::PebblingResult(const PebblingModel& m, Tactic t)
      : IpdrResult(m), model(m), tactic(t)
  {
    assert(t == Tactic::decrement || t == Tactic::increment);
  }

  PebblingResult::PebblingResult(
      const IpdrResult& r, const PebblingModel& m, Tactic t)
      : IpdrResult(r), model(m), tactic(t)
  {
    rows.resize(0);
    for (const PdrResult& r : original)
    {
      tabulate::Table::Row_t res_row = table_row(r);
      assert(res_row.size() == header().size());
      rows.push_back(res_row);
    }
  }

  void PebblingResult::add_to_table(tabulate::Table& t) const
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
      assert(tactic == Tactic::decrement &&
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

    table.add_row({ "runtime", "max constraint with invariant", "level",
        "min constraint with strategy", "length" });
    add_to_table(table);

    out << table << std::endl;
    auto latex = tabulate::LatexExporter().dump(table);
    out << latex << std::endl;
  }

  tabulate::Table::Row_t PebblingResult::table_row(const PdrResult& r)
  {
#warning takes highest constrained invariant, and lowest number of pebbles used. Vs lowest level and minimal length?? Vs store latest
    total.time += r.time;

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

      if (tactic == Tactic::decrement)
      {
        assert(++invariants <= 1);
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

      if (tactic == Tactic::increment)
      {
        assert(++traces <= 1);
      }
    }

    return row;
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
    return { "constraint", "pebbled", "invariant index", "trace length",
      "time" };
  }

  std::string PebblingResult::process_trace(const PdrResult& res) const
  {
    std::cerr << "Pebbling processing" << std::endl;
#warning double initial state in experiment results
    using fmt::format;
    using std::string_view;
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
    std::vector<std::string> lits = model.vars.names();
    std::sort(lits.begin(), lits.end());

    size_t longest =
        std::max_element(lits.begin(), lits.end(), str_size_cmp)->size();

    Table t;
    // Write top row
    {
      Table::Row_t trace_header = { "", "marked" };
      trace_header.insert(trace_header.end(), lits.begin(), lits.end());
      t.add_row(trace_header);
    }

    auto make_row = [&lits, longest](string a, string b, const State& s)
    {
      std::vector<std::string> r = state::marking(s, lits, longest);
      r.insert(r.begin(), b);
      r.insert(r.begin(), a);
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write initial state
    {
      expr_vector initial_state = model.get_initial();
      Table::Row_t initial_row  = make_row("I", "0", State(initial_state));
      t.add_row(initial_row);
    }

    // Write strategy states
    {
      unsigned marked = model.get_f_pebbles();
      for (size_t i = 0; i < res.trace().states.size(); i++)
      {
        const z3::expr_vector& s = res.trace().states[i];
        unsigned pebbled         = state::no_marked(s);
        marked                   = std::max(marked, pebbled);
        Table::Row_t row_marking =
            make_row(std::to_string(i), std::to_string(pebbled), s);
        t.add_row(row_marking);
      }
      ss << format("Strategy for {} pebbles", marked) << std::endl << std::endl;
    }

    // Write final state
    {
      expr_vector final_state = model.n_property;
      Table::Row_t final_row  = make_row(
           "F", format("{}", model.get_f_pebbles()), State(final_state));
      t.add_row(final_row);
    }

    t.format().font_align(tabulate::FontAlign::right);

    ss << tabulate::MarkdownExporter().dump(t);
    return ss.str();
  }
} // namespace pdr::pebbling