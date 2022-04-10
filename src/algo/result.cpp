#include "result.h"
#include <array>
#include <cassert>
#include <memory>
#include <string>

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
  using iterator          = Result::iterator;
  using iterator_category = std::forward_iterator_tag;
  using difference_type   = std::ptrdiff_t;
  using value_type        = State;
  using pointer           = std::shared_ptr<State>;
  using reference         = State&;

  iterator::iterator(pointer ptr) : m_ptr(ptr) {}
  reference iterator::operator*() const { return *m_ptr; }
  pointer iterator::operator->() { return m_ptr; }

  iterator& iterator::operator++()
  {
    m_ptr = m_ptr->prev;
    return *this;
  }

  iterator iterator::operator++(int)
  {
    iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const iterator& a, const iterator& b)
  {
    return a.m_ptr == b.m_ptr;
  }

  bool operator!=(const iterator& a, const iterator& b)
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
    rv[4] = to_string(total_time);

    return rv;
  }

  std::string_view Result::strategy_string(const PebblingModel& model)
  {
    using string_vec = std::vector<std::string>;
    using fmt::format;
    using std::string_view;
    using z3::expr;
    using z3::expr_vector;

    if (str != "")
      return str;

    if (has_invariant())
    {
      if (constraint)
        str = format("No strategy for {}\n", *constraint);
      else
        str = "No strategy\n";
      return str;
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
    size_t largest =
        std::max_element(lits.begin(), lits.end(), str_size_cmp)->size();

    TextTable t('|');
    {
      string_vec header = { "", "" };
      header.insert(header.end(), lits.begin(), lits.end());
      t.addRow(header);
    }

    auto row = [&lits, largest](string a, string b, const State& s)
    {
      string_vec rv = s.marking(lits, largest);
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
      unsigned i = 0;
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

    return str;
  }

  iterator Result::begin()
  {
    if (has_invariant())
      return end();
    // we have a Trace
    const Trace& trace = std::get<Trace>(output);
    assert(trace.cleaned == false);
    return iterator(trace.states_ll);
  }
  iterator Result::end() { return iterator(nullptr); }

  // Results members
  //
  Results::Results(const PebblingModel& m) : model(m)
  {
    ResultRow header = { "constraint", "pebbles used", "invariant index",
      "trace length", "Total time" };
    for (unsigned i = 0; i < header.size(); i++)
      table.setAlignment(i, TextTable::Alignment::RIGHT);

    table.addRow(header);
  }

  void Results::show(std::ostream& out) const
  {
    out << table << std::endl << std::endl;

    for (const std::string& trace : traces)
      out << trace << std::endl;
  }

  Results& operator<<(Results& rs, Result& r)
  {
    string trace;
    trace = r.strategy_string(rs.model);
    rs.table.addRow(r.listing());

    rs.traces.push_back(trace);

    return rs;
  }
} // namespace pdr
