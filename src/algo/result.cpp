#include "result.h"
#include "expr.h"
#include "obligation.h"
#include "pdr-model.h"
#include "pebbling-model.h"
#include "z3-ext.h"

#include <TextTable.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include <variant>

namespace pdr
{
  using std::get;
  using std::shared_ptr;
  using std::string;
  using std::vector;
  using z3ext::LitStr;
  using Invariant  = PdrResult::Invariant;
  using Trace      = PdrResult::Trace;
  using TraceState = PdrResult::Trace::TraceState;
  using TraceVec   = PdrResult::Trace::TraceVec;

  namespace // helper
  {
    bool str_size_cmp(string_view a, string_view b)
    {
      return a.size() < b.size();
    };

    // convert a linked list of PdrStates
    TraceVec make_trace_marking(shared_ptr<const PdrState> s)
    {
      TraceVec rv;
      while (s)
      {
        vector<LitStr> state;
        for (z3::expr const& e : s->cube)
          state.push_back(z3ext::LitStr(e));
        rv.push_back(state);

        s = s->prev;
      }
      return rv;
    }

    // return greatest amount of positive literals among each vector in trace
    size_t greatest_marking(TraceVec const& trace)
    {
      size_t rv{ 0 };
      for (TraceState const& s : trace)
      {
        rv = std::max(rv, state::n_marked(s));
      }
      return rv;
    }
  } // namespace

  // Result::Invariant and Trace members
  //
  Invariant::Invariant(int l) : level(l) {}

  Trace::Trace() : length{ 0 }, n_marked{ 0 } {}
  // Trace::Trace(Trace const& t)
  //     : states(t.states), length(t.length), n_marked(t.n_marked)
  // {
  // }
  Trace::Trace(unsigned l) : length{ l }, n_marked{ 0 } {}
  Trace::Trace(shared_ptr<const PdrState> s)
      : states(make_trace_marking(s)),
        length(states.size()),
        n_marked(greatest_marking(states))
  {
  }
  Trace::Trace(TraceVec const& trace_states)
      : states(trace_states),
        length(trace_states.size()),
        n_marked(greatest_marking(trace_states))
  {
  }

  // Result members
  //
  PdrResult::PdrResult(std::variant<Invariant, Trace> o) : output(o) {}

  PdrResult::PdrResult(std::shared_ptr<PdrState> s) : output(Trace(s)) {}

  PdrResult::PdrResult(int level) : output(Invariant(level)) {}

  PdrResult::PdrResult(TraceVec const& trace) : output(Trace(trace)) {}

  // build functions
  PdrResult PdrResult::found_trace(TraceVec const& trace)
  {
    return PdrResult(trace);
  }
  PdrResult PdrResult::found_trace(std::shared_ptr<PdrState> s)
  {
    return PdrResult(s);
  }
  PdrResult PdrResult::found_trace(PdrState&& s)
  {
    return PdrResult(std::make_shared<PdrState>(s));
  }
  PdrResult PdrResult::incomplete_trace(unsigned length)
  {
    return PdrResult(Trace(length));
  }
  PdrResult& PdrResult::with_duration(double t)
  {
    time = t;
    return *this;
  }

#warning TODO int level to size_t
  PdrResult PdrResult::found_invariant(int level) { return PdrResult(level); }

  PdrResult PdrResult::empty_true() { return PdrResult(-1); }
  PdrResult PdrResult::empty_false() { return PdrResult(nullptr); }

  void PdrResult::append_final(z3::expr_vector const& f)
  {
    assert(has_trace());
    TraceState final;
    for (z3::expr const& e : f)
      final.emplace_back(e);

    trace().states.push_back(final);
  }

  PdrResult::operator bool() const { return has_invariant(); }
  bool PdrResult::has_invariant() const
  {
    return std::holds_alternative<Invariant>(output);
  }
  bool PdrResult::has_trace() const
  {
    return std::holds_alternative<Trace>(output);
  }

  Invariant const& PdrResult::invariant() const
  {
    return get<Invariant>(output);
  }
  Trace const& PdrResult::trace() const { return get<Trace>(output); }
  Invariant& PdrResult::invariant() { return get<Invariant>(output); }
  Trace& PdrResult::trace() { return get<Trace>(output); }

  PdrResult::ResultRow PdrResult::listing() const
  {
    using fmt::format;
    using std::to_string;

    if (has_invariant())
      return { format("F_{}", invariant().level), "", to_string(time) };
    else
      return { "", to_string(trace().length), to_string(time) };
  }

  tabulate::Table PdrResult::get_table() const
  {
    tabulate::Table T;
    {
      T.add_row({ PdrResult::fields.cbegin(), PdrResult::fields.cend() });
      auto const row2 = listing();
      T.add_row({ row2.cbegin(), row2.cend() });
    }
    return T;
  }

  // IpdrResult
  // Public members
  //

  IpdrResult::IpdrResult(IModel const& m)
      : IpdrResult(m.vars.names(), m.vars.names_p())
  {
  }
  IpdrResult::IpdrResult(vector<string> const& v, vector<string> const& vp)
      : vars(v), vars_p(vp)
  {
  }

  IpdrResult::~IpdrResult() {}

  void IpdrResult::reset() { pdr_summaries.resize(0); }

  double IpdrResult::get_total_time() const { return total_time; }

  vector<double> IpdrResult::g_times() const
  {
    vector<double> times;
    std::transform(original.begin(), original.end(), std::back_inserter(times),
        [](PdrResult const& r) { return r.time; });
    return times;
  }

  tabulate::Table IpdrResult::summary_table() const
  {
    tabulate::Table t;
    t.add_row(summary_header());

    for (auto row : pdr_summaries)
    {
      if (row.size() == t.row(0).size() - 1)
        row.push_back(""); // append no inc_time
      else 
      {
        assert(row.size() == t.row(0).size());
      }
      t.add_row(row);
    }

    tabulate::Table::Row_t total;
    {
      total.resize(pdr_summaries.at(0).size()); // match no. columns

      vector<double> times = g_times();
      // assert(total_time == std::accumulate(times.begin(), times.end(), 0.0));
      total.back() = fmt::format("{}", total_time);
      t.add_row(total);
    }

    return t;
  }

  tabulate::Table IpdrResult::total_table() const
  {
    using fmt::format;
    using std::to_string;

    tabulate::Table table;
    table.format().font_align(tabulate::FontAlign::right);

    table.add_row(total_header());
    table.add_row(total_row());

    return table;
  }

  std::string IpdrResult::all_traces() const
  {
    std::stringstream ss;
    for (std::string_view trace : traces)
      ss << trace << std::endl;
    return ss.str();
  }

  IpdrResult& IpdrResult::add(PdrResult const& r)
  {
    tabulate::Table::Row_t res_row = process_result(r);
    assert(res_row.size() == summary_header().size());
    pdr_summaries.push_back(res_row);

    return *this;
  }

  void IpdrResult::append_inc(double time)
  {
    total_time += time;
    inc_times.push_back(time);
    pdr_summaries.back().push_back(std::to_string(time));
  }

  const tabulate::Table::Row_t IpdrResult::process_result(PdrResult const& r)
  {
    original.push_back(r);
    traces.push_back(process_trace(r));
    total_time += r.time;

    tabulate::Table::Row_t row;

    PdrResult::ResultRow listing = r.listing();
    row.assign(listing.begin(), listing.end());

    assert(row.size() == PdrResult::fields.size());
    return row;
  }

  // Private members
  //
  std::string IpdrResult::process_trace(PdrResult const& res) const
  {
    return result::trace_table(res, vars, vars_p);
  }

  const tabulate::Table::Row_t IpdrResult::summary_header() const
  {
    tabulate::Table::Row_t rv;
    rv.assign(PdrResult::fields.cbegin(), PdrResult::fields.cend());
    rv.push_back("incremental time");
    return rv;
  }

  // GENERAL
  //
  namespace result
  {
    std::string trace_table(PdrResult const& res,
        std::vector<std::string> const& vars,
        std::vector<std::string> const& vars_p)
    {
      using fmt::format;
      using std::to_string;
      using tabulate::Table;

      if (res.has_invariant())
        return "Invariant, no trace.";

      // process trace
      std::stringstream ss;

      size_t longest =
          std::max_element(vars.begin(), vars.end(), str_size_cmp)->size();

      Table t;
      // Write top row
      {
        Table::Row_t trace_header = {
          "",
        };
        trace_header.insert(trace_header.end(), vars.begin(), vars.end());
        t.add_row(trace_header);
      }

      auto make_row =
          [longest](string i, TraceState const& s, const vector<string>& names)
      {
        vector<string> r = state::marking(s, names, longest);
        r.insert(r.begin(), string(i));
        Table::Row_t rv;
        rv.assign(r.begin(), r.end());
        return rv;
      };

      // Write trace states
      {
        size_t N = res.trace().states.size();
        for (size_t i = 0; i < N; i++)
        {
          TraceState const& s = res.trace().states[i];
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

#warning z3pdr result uses only vars, vars_p gives `?` in final result
          Table::Row_t row_marking =
              make_row(index_str, s, (i < N - 1 ? vars : vars_p));
          t.add_row(row_marking);
        }
      }

      t.format().font_align(tabulate::FontAlign::right);

      ss << tabulate::MarkdownExporter().dump(t);
      return ss.str();
    }
  } // namespace result

  namespace state
  {
    size_t n_marked(PdrResult::Trace::TraceState const& s)
    {
      return std::accumulate(s.cbegin(), s.cend(), size_t{ 0 },
          [](size_t a, LitStr const& l) { return a + l.sign; });
    }

    // return strings that mark whether every state in header a positive or
    // negative literal
    vector<string> marking(
        TraceState const& state, vector<string> header, unsigned width)
    {
      assert(std::is_sorted(header.begin(), header.end()));

      vector<string> rv(header.size(), "?");
      for (z3ext::LitStr const& lit : state)
      {
        auto it = std::lower_bound(header.begin(), header.end(), lit,
            [](std::string_view h, LitStr const& l) { return h < l.atom; });

        if (it != header.end() && *it == lit.atom) // it points to s
        {
          string fill_X           = fmt::format("{:X^{}}", "", width);
          rv[it - header.begin()] = lit.sign ? fill_X : "";
        }
      }

      return rv;
    }
  } // namespace state

} // namespace pdr
