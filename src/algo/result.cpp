#include "result.h"
#include "output.h"
#include <TextTable.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <numeric>
#include <string>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>

namespace pdr
{
  using std::get;
  using std::make_shared;
  using std::optional;
  using std::shared_ptr;
  using std::string;

  // Result::Invariant and Trace members
  //
  using Invariant = Result::Invariant;
  using Trace     = Result::Trace;
  Invariant::Invariant(int l) : level(l) {}
  Invariant::Invariant(optional<unsigned> c, int l) : constraint(c), level(l) {}

  Trace::Trace() : states_ll(nullptr), length(0), marked(0) {}
  Trace::Trace(unsigned l, unsigned m)
      : states_ll(nullptr), length(l), marked(m)
  {
  }
  Trace::Trace(shared_ptr<State> s) : states_ll(s), length(0), marked(0) {}

  // Result::iterator members
  //
  using const_iterator  = Result::const_iterator;
  using difference_type = std::ptrdiff_t;
  using value_type      = State;
  using const_pointer   = std::shared_ptr<const State>;
  using const_reference = const State&;

  const_iterator::const_iterator(const_pointer ptr) : m_ptr(ptr) {}
  const_reference const_iterator::operator*() const { return *m_ptr; }
  const_pointer const_iterator::operator->() { return m_ptr; }

  const_iterator& const_iterator::operator++()
  {
    m_ptr = m_ptr->prev;
    return *this;
  }

  const_iterator const_iterator::operator++(int)
  {
    const_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const const_iterator& a, const const_iterator& b)
  {
    return a.m_ptr == b.m_ptr;
  }

  bool operator!=(const const_iterator& a, const const_iterator& b)
  {
    return a.m_ptr != b.m_ptr;
  }

  // Result members
  //
  Result::Result(optional<unsigned> constr, std::shared_ptr<State> s)
      : constraint(constr), output(Trace(s))
  {
  }

  Result::Result(optional<unsigned> constr, int l)
      : constraint(constr), output(Invariant(constr, l))
  {
  }

  Result Result::found_trace(
      optional<unsigned> constraint, std::shared_ptr<State> s)
  {
    return Result(constraint, s);
  }

  Result Result::found_trace(optional<unsigned> constraint, State&& s)
  {
    return Result(constraint, std::make_shared<State>(s));
  }

#warning TODO int level to size_t
  Result Result::found_invariant(optional<unsigned> constraint, int level)
  {
    return Result(constraint, level);
  }

  Result Result::empty_true() { return Result(0, -1); }
  Result Result::empty_false() { return Result(0, nullptr); }

  Result::operator bool() const { return has_invariant(); }
  bool Result::has_invariant() const { return output.index() == 0; }
  bool Result::has_trace() const { return output.index() == 1; }

  const Invariant& Result::invariant() const { return get<Invariant>(output); }
  const Trace& Result::trace() const { return get<Trace>(output); }
  Invariant& Result::invariant() { return get<Invariant>(output); }
  Trace& Result::trace() { return get<Trace>(output); }

  void Result::clean_trace()
  {
    assert(has_trace()); // we must have a trace

    Trace& trace = get<Trace>(output);
    assert(trace.states_ll);
    trace.states_ll.reset();
  }

  Result::ResultRow Result::listing() const
  {
    using fmt::format;
    using std::to_string;

    // header =
    // { "constraint", "pebbles used", "invariant index", "trace length",
    // "Total time" }
    ResultRow rv;
    rv[0] = constraint ? to_string(*constraint) : "none";
    if (has_invariant())
    {
      rv[1] = "";
      rv[2] = format("F_{}", invariant().level);
      rv[3] = "";
    }
    else
    {
      rv[1] = to_string(trace().marked);
      rv[2] = "";
      rv[3] = to_string(trace().length);
    }
    rv[4] = to_string(time);

    return rv;
  }

  std::string_view Result::string_rep() const
  {
    assert(finalized);
    return str;
  }

  void Result::finalize(const pebbling::Model& model)
  {
#warning double initial state in experiment results
    using fmt::format;
    using std::string_view;
    using tabulate::Table;
    using z3::expr;
    using z3::expr_vector;

    if (finalized)
      return;
    finalized = true;

    if (has_invariant())
    {
      if (constraint)
        str = format("No strategy for {}\n", *constraint);
      else
        str = "No strategy\n";
      return;
    }

    std::vector<std::string> lits;
    {
      expr_vector z3_lits = model.vars.currents();
      std::transform(z3_lits.begin(), z3_lits.end(), std::back_inserter(lits),
          [](expr l) { return l.to_string(); });
      std::sort(lits.begin(), lits.end());
    }

    auto str_size_cmp = [](string_view a, string_view b)
    { return a.size() < b.size(); };
    size_t longest =
        std::max_element(lits.begin(), lits.end(), str_size_cmp)->size();

    Table t;
    {
      Table::Row_t header = { "", "marked" };
      header.insert(header.end(), lits.begin(), lits.end());
      t.add_row(header);
    }

    auto row = [&lits, longest](string a, string b, const State& s)
    {
      std::vector<std::string> r = s.marking(lits, longest);
      r.insert(r.begin(), b);
      r.insert(r.begin(), a);
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write initial state
    {
      expr_vector initial_state = model.get_initial();
      Table::Row_t initial_row  = row("I", "0", State(initial_state));
      t.add_row(initial_row);
    }

    // Write strategy states
    {
      trace().marked = model.get_f_pebbles();
      unsigned i     = 0;
      for (const State& s : *this)
      {
        i++;
        unsigned pebbles = s.no_marked();
        trace().marked   = std::max(trace().marked, pebbles);
        Table::Row_t row_marking =
            row(std::to_string(i), std::to_string(pebbles), s);
        t.add_row(row_marking);
      }
      trace().length = i + 1;
    }

    // Write final state
    {
      expr_vector final_state = model.n_property.currents();
      Table::Row_t final_row =
          row("F", format("{}", model.get_f_pebbles()), State(final_state));
      t.add_row(final_row);
    }

    t.format().font_align(tabulate::FontAlign::right);

    std::stringstream ss;
    ss << "Strategy for " << trace().marked << " pebbles" << std::endl
       << std::endl
       << tabulate::MarkdownExporter().dump(t);
    str = ss.str();

    clean_trace(); // information is stored in string, deallocate trace
  }

  const_iterator Result::begin()
  {
    if (has_invariant())
      return end();
    // we have a Trace
    const Trace& trace = std::get<Trace>(output);
    assert(trace.states_ll);
    return const_iterator(trace.states_ll);
  }
  const_iterator Result::end() { return const_iterator(nullptr); }

  // Results members
  //
  Results::Results(const pebbling::Model& m) : model(m) {}

  Results::~Results() {}

  tabulate::Table Results::new_table() const
  {
    tabulate::Table t;
    t.add_row(header);
    return t;
  }

  void Results::reset()
  {
    rows.resize(0);
    traces.resize(0);
  }

  void Results::show(std::ostream& out) const
  {
    tabulate::Table t = raw_table();
    out << t << std::endl << std::endl;

    for (const std::string& trace : traces)
      out << trace << std::endl;
  }

  Results& Results::add(Result& r)
  {
    string trace(r.string_rep());
    Result::ResultRow listing = r.listing();

    {
      tabulate::Table::Row_t row;
      row.assign(listing.begin(), listing.end());
      rows.push_back(row);
    }

    original.push_back(r);
    traces.push_back(trace);

    return *this;
  }

  std::vector<double> Results::g_times() const
  {
    std::vector<double> times;
    std::transform(original.begin(), original.end(), std::back_inserter(times),
        [](const Result& r) { return r.time; });
    return times;
  }

  tabulate::Table Results::raw_table() const
  {
    tabulate::Table t = new_table();
    {
      for (const auto& row : rows)
        t.add_row(row);
      tabulate::Table::Row_t total;
      total.resize(rows.at(0).size());

      std::vector<double> times = g_times();
      total.back() =
          fmt::format("{}", std::accumulate(times.begin(), times.end(), 0.0));
      t.add_row(total);
    }

    return t;
  }

  void Results::show_traces(std::ostream& out) const
  {
    for (const std::string& t : traces)
      out << t << std::endl;
  }

  Results& operator<<(Results& rs, Result& r) { return rs.add(r); }

  // ExperimentResults members
  //
  ExperimentResults::ExperimentResults(const pebbling::Model& m, Tactic t)
      : Results(m), tactic(t)
  {
    assert(t == Tactic::decrement || t == Tactic::increment);
  }

  ExperimentResults::ExperimentResults(const Results& r, Tactic t)
      : ExperimentResults(r.model, t)
  {
    rows     = r.rows;
    original = r.original;
    traces   = r.traces;
    for (const Result& r : original)
      acc_update(r);
  }

  ExperimentResults::Data_t ExperimentResults::get_total() const
  {
    return { total_time, max_inv, min_strat };
  }

  void ExperimentResults::add_to(tabulate::Table& t) const
  {
    using fmt::format;
    using std::to_string;

    auto [time, max_inv, min_strat] = get_total();
    string time_str                 = format("{}", time);

    string inv_constr, inv_level;
    if (max_inv)
    {
      inv_constr = to_string(max_inv->constraint.value());
      inv_level  = format("F_{}", max_inv->level);
    }
    else
    {
      assert(tactic == Tactic::decrement &&
             min_strat->marked == model.get_f_pebbles());
      // the maximal invariant did not need to be considered and added
      inv_constr = "strategy uses minimal";
      inv_level  = "--";
    }

    string trace_marked, trace_length;
    if (min_strat)
    {
      trace_marked = to_string(min_strat->marked);
      trace_length = to_string(min_strat->length);
    }
    else
    {
      assert(max_inv->constraint == model.n_nodes());
      // never encountered a strategy, no. nodes is the invariant
      trace_marked = "no strategy";
      trace_length = "--";
    }

    t.add_row({ time_str, inv_constr, inv_level, trace_marked, trace_length });
  }

  void ExperimentResults::show(std::ostream& out) const
  {
    using fmt::format;
    using std::to_string;

    tabulate::Table table;
    table.format().font_align(tabulate::FontAlign::right);

    table.add_row({ "runtime", "max constraint with invariant", "level",
        "min constraint with strategy", "length" });
    add_to(table);

    out << table << std::endl;
    auto latex = tabulate::LatexExporter().dump(table);
    out << latex << std::endl;
  }

  void ExperimentResults::show_raw(std::ostream& out) const
  {
    Results::show(out);
  }

  void ExperimentResults::acc_update(const Result& r)
  {
    total_time += r.time;

    if (r.has_invariant())
    {
      assert(r.invariant().constraint);
      if (max_inv)
        max_inv = std::max(*max_inv, r.invariant(),
            [](Invariant a, Invariant b)
            { return a.constraint < b.constraint; });
      else
        max_inv = r.invariant();
    }
    else
    {
      if (min_strat)
        min_strat = std::min(*min_strat, r.trace(),
            [](const Trace& a, const Trace& b) { return a.marked < b.marked; });
      else
        min_strat = r.trace();
    }
  }

  ExperimentResults& ExperimentResults::add(Result& r)
  {
    acc_update(r);
    Results::add(r);
    return *this;
  }

  ExperimentResults& operator<<(ExperimentResults& ars, Result& r)
  {
    return ars.add(r);
  }
} // namespace pdr
