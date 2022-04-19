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

namespace pdr
{
  using std::get;
  using std::make_shared;
  using std::optional;
  using std::shared_ptr;
  using std::string;

  using ResultRow = std::array<string, 5>;

  // Result::Invariant and Trace members
  //
  using Invariant = Result::Invariant;
  using Trace     = Result::Trace;
  Invariant::Invariant(int l) : level(l) {}

  Trace::Trace() : states_ll(nullptr), length(0), marked(0), cleaned(false) {}
  Trace::Trace(shared_ptr<State> s)
      : states_ll(s), length(0), marked(0), cleaned(false)
  {
  }

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
      : constraint(constr), output(Invariant(l))
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
    trace.cleaned = true;
  }

  ResultRow Result::listing() const
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
    using string_vec = std::vector<std::string>;
    using fmt::format;
    using std::string_view;
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

    string_vec lits;
    {
      expr_vector z3_lits = model.lits.currents();
      std::transform(z3_lits.begin(), z3_lits.end(), std::back_inserter(lits),
          [](expr l) { return l.to_string(); });
      std::sort(lits.begin(), lits.end());
    }

    auto str_size_cmp = [](string_view a, string_view b)
    { return a.size() < b.size(); };
    size_t longest =
        std::max_element(lits.begin(), lits.end(), str_size_cmp)->size();

    TextTable t('|');
    {
      string_vec header = { "", "" };
      header.insert(header.end(), lits.begin(), lits.end());
      t.addRow(header);
    }

    auto row = [&lits, longest](string a, string b, const State& s)
    {
      string_vec rv = s.marking(lits, longest);
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
      trace().marked = model.get_f_pebbles();
      unsigned i     = 0;
      for (const State& s : *this)
      {
        i++;
        unsigned pebbles = s.no_marked();
        trace().marked   = std::max(trace().marked, pebbles);
        string_vec row_marking =
            row(std::to_string(i), format("No. pebbled = {}", pebbles), s);
        t.addRow(row_marking);
      }
      trace().length = i + 1;
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
    ss << "Strategy for " << trace().marked << " pebbles" << std::endl;
    ss << t;
    str = ss.str();

    clean_trace(); // information is stored in string, deallocate trace
  }

  const_iterator Result::begin()
  {
    if (has_invariant())
      return end();
    // we have a Trace
    const Trace& trace = std::get<Trace>(output);
    assert(trace.cleaned == false);
    return const_iterator(trace.states_ll);
  }
  const_iterator Result::end() { return const_iterator(nullptr); }

  // Results members
  //
  Results::Results(const pebbling::Model& m) : model(m) {}

  Results::~Results() {}

  TextTable Results::new_table() const
  {
    TextTable t;
    for (unsigned i = 0; i < header.size(); i++)
      t.setAlignment(i, TextTable::Alignment::RIGHT);
    t.addRow(header);

    return t;
  }

  void Results::reset()
  {
    rows.resize(0);
    traces.resize(0);
  }

  void Results::show(std::ostream& out) const
  {
    TextTable t = new_table();
    for (const auto& row : rows)
      t.addRow(row);
    out << t << std::endl << std::endl;

    for (const std::string& trace : traces)
      out << trace << std::endl;
  }

  Results& Results::add(Result& r)
  {
    string trace(r.string_rep());
    rows.push_back(r.listing());
    traces.push_back(trace);
    original.push_back(r);

    return *this;
  }

  Results& operator<<(Results& rs, Result& r) { return rs.add(r); }

  std::vector<double> Results::extract_times() const
  {
    std::vector<double> times;
    std::transform(original.begin(), original.end(), std::back_inserter(times),
        [](const Result& r) { return r.time; });
    return times;
  }

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

  void ExperimentResults::add_to(tabulate::Table& t) const
  {
    using fmt::format;

    const auto none = original.crend();
    auto max_inv    = original.crbegin(); // last element
    auto min_strat  = original.crbegin(); // last element

    if (tactic == Tactic::decrement)
    {
      if (min_strat->has_trace() &&
          min_strat->trace().marked == model.get_f_pebbles())
        max_inv = none; // crbegin is min strategy. no invariant
      else
        min_strat++; // crbegin is max invariant. crbegin+1 is min strategy
    }
    else if (tactic == Tactic::increment)
    {
      if (max_inv->constraint == model.n_nodes())
        min_strat = none; // crbegin is invariant. no strategy
      else
        max_inv++; // crbegin is strat. crbegin+1 is invariant
    }
    else
      assert(false && "Only inc or dec for experiment");

    std::optional<unsigned> constr, Flevel, marked, length;
    if (max_inv != none)
    {
      constr = max_inv->constraint;
      Flevel = max_inv->invariant().level;
    }
    if (min_strat != none)
    {
      marked = min_strat->trace().marked;
      length = min_strat->trace().length;
    }

    t.add_row({ format("{}", total_time),
        constr ? format("{}", constr.value()) : "strategy uses least possible",
        Flevel ? format("F_{}", Flevel.value()) : "no F_i",
        marked ? format("{}", marked.value()) : "no strategy",
        length ? format("{}", length.value()) : "" });

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
      invariant_level = std::min(invariant_level, r.invariant().level);
    else
    {
      length = std::min(length, r.trace().length);
      marked = std::min(marked, r.trace().marked);
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
