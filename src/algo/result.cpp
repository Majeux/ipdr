#include "result.h"
#include "obligation.h"
#include "output.h"
#include "pebbling-model.h"

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
  using std::vector;

  // Result::Invariant and Trace members
  //
  using Invariant = PdrResult::Invariant;
  using Trace     = PdrResult::Trace;
  Invariant::Invariant(int l) : level(l) {}

  Trace::Trace() : length(0) {}
  Trace::Trace(unsigned l) : length(l) {}
  Trace::Trace(shared_ptr<const State> s) : length(0)
  {
    while (s)
    {
      states.push_back(s->cube);
      length++;
      s = s->prev;
    }
  }

  // Result members
  //
  PdrResult::PdrResult(std::shared_ptr<State> s) : output(Trace(s)) {}

  PdrResult::PdrResult(int l) : output(Invariant(l)) {}

  PdrResult PdrResult::found_trace(std::shared_ptr<State> s)
  {
    return PdrResult(s);
  }

  PdrResult PdrResult::found_trace(State&& s)
  {
    return PdrResult(std::make_shared<State>(s));
  }

#warning TODO int level to size_t
  PdrResult PdrResult::found_invariant(int level) { return PdrResult(level); }

  PdrResult PdrResult::empty_true() { return PdrResult(-1); }
  PdrResult PdrResult::empty_false() { return PdrResult(nullptr); }

  PdrResult::operator bool() const { return has_invariant(); }
  bool PdrResult::has_invariant() const { return output.index() == 0; }
  bool PdrResult::has_trace() const { return output.index() == 1; }

  const Invariant& PdrResult::invariant() const
  {
    return get<Invariant>(output);
  }
  const Trace& PdrResult::trace() const { return get<Trace>(output); }
  Invariant& PdrResult::invariant() { return get<Invariant>(output); }
  Trace& PdrResult::trace() { return get<Trace>(output); }

  PdrResult::ResultRow PdrResult::listing() const
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
      rv[1] = "todo: remove";
      rv[2] = "";
      rv[3] = to_string(trace().length);
    }
    rv[4] = to_string(time);

    return rv;
  }

  // IpdrResult
  // auxiliary
  //

  // return strings that mark whether every state in header a positive or
  // negative literal
  vector<string> marking(const State& s, vector<string> header, unsigned width)
  {
    vector<string> rv(header.size(), "?");
    for (const z3::expr& e : s.cube)
    {
      string s = e.is_not() ? e.arg(0).to_string() : e.to_string();
      auto it  = std::lower_bound(header.begin(), header.end(), s);
      if (it != header.end() && *it == s) // it points to s
      {
        string fill_X           = fmt::format("{:X^{}}", "", width);
        rv[it - header.begin()] = e.is_not() ? "" : fill_X;
      }
    }

    return rv;
  }

  // members
  //
  IpdrResult::~IpdrResult() {}

  tabulate::Table IpdrResult::new_table() const
  {
    tabulate::Table t;
    t.add_row(header());
    return t;
  }

  void IpdrResult::reset()
  {
    rows.resize(0);
    traces.resize(0);
  }

  void IpdrResult::show(std::ostream& out) const
  {
    tabulate::Table t = raw_table();
    out << t << std::endl << std::endl;

    for (const std::string& trace : traces)
      out << trace << std::endl;
  }

  IpdrResult& IpdrResult::add(PdrResult& r)
  {
    PdrResult::ResultRow listing = r.listing();
    {
      tabulate::Table::Row_t row;
      row.assign(listing.begin(), listing.end());
      rows.push_back(row);
    }

    original.push_back(r);
    traces.push_back(process(r));

    return *this;
  }

  std::vector<double> IpdrResult::g_times() const
  {
    std::vector<double> times;
    std::transform(original.begin(), original.end(), std::back_inserter(times),
        [](const PdrResult& r) { return r.time; });
    return times;
  }

  tabulate::Table IpdrResult::raw_table() const
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

  void IpdrResult::show_traces(std::ostream& out) const
  {
    for (const std::string& t : traces)
      out << t << std::endl;
  }

  IpdrResult& operator<<(IpdrResult& rs, PdrResult& r) { return rs.add(r); }

  const tabulate::Table::Row_t IpdrResult::header() const
  {
    return { "invariant index", "trace length", "time" };
  }

  namespace pebbling
  {
    // PebblingResult members
    //
    PebblingResult::PebblingResult(const PebblingModel& m, Tactic t)
        : IpdrResult(), model(m), tactic(t)
    {
      assert(t == Tactic::decrement || t == Tactic::increment);
    }

    PebblingResult::PebblingResult(
        const IpdrResult& r, const PebblingModel& m, Tactic t)
        : IpdrResult(r), model(m), tactic(t)
    {
      for (const PdrResult& r : original)
        acc_update(r);
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

      t.add_row(
          { time_str, inv_constr, inv_level, trace_marked, trace_length });
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

    void PebblingResult::show_raw(std::ostream& out) const
    {
      IpdrResult::show(out);
    }

    unsigned total_pebbled(const Trace& t)
    {
      unsigned pebbled = 0;
      for (const z3::expr_vector& s : t.states)
        pebbled = std::max(pebbled, no_marked(s));
      return pebbled;
    }

    void PebblingResult::acc_update(const PdrResult& r)
    {
#warning takes highest constrained invariant, and lowest number of pebbles used. Vs lowest level and minimal length?? Vs store latest
      total.time += r.time;

      if (r.has_invariant()) // only happens multiple times in increasing
      {
        invariants++;
        if (tactic == Tactic::decrement)
        {
          assert(invariants <= 1);
        }

        optional<unsigned> constraint = model.get_max_pebbles();
        assert(constraint);
		// track the invariant with the lowest constraint
        if (!total.inv || constraint > total.inv->constraint)
          total.inv = { r.invariant(), constraint };
      }
      else // only happens multiple times in decreasing
      {
        traces++;
        if (tactic == Tactic::increment)
        {
          assert(traces <= 1);
        }

		// track the strategy with the lowest amount of pebbles
        unsigned pebbled = total_pebbled(r.trace());
        if (!total.strategy || pebbled < total.strategy->pebbled)
          total.strategy = { r.trace(), pebbled };
      }
    }

    PebblingResult& PebblingResult::add(PdrResult& r)
    {
      acc_update(r);
      IpdrResult::add(r);
      return *this;
    }

    PebblingResult& operator<<(PebblingResult& ars, PdrResult& r)
    {
      return ars.add(r);
    }

    const PebblingResult::Data_t& PebblingResult::get_total() const
	{
		return total;
	}

    // Private Members
    //
    std::string PebblingResult::process(const PdrResult& res) const
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

      auto str_size_cmp = [](string_view a, string_view b)
      { return a.size() < b.size(); };
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
        std::vector<std::string> r = marking(s, lits, longest);
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
          unsigned pebbled         = no_marked(s);
          marked                   = std::max(marked, pebbled);
          Table::Row_t row_marking =
              make_row(std::to_string(i), std::to_string(pebbled), s);
          t.add_row(row_marking);
        }
        ss << format("Strategy for {} pebbles", marked) << std::endl
           << std::endl;
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
  } // namespace pebbling
} // namespace pdr
