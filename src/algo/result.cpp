#include "result.h"
#include "expr.h"
#include "obligation.h"
#include "pdr-model.h"
#include "pebbling-model.h"

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

  namespace // helper
  {
    bool str_size_cmp(string_view a, string_view b)
    {
      return a.size() < b.size();
    };
  } // namespace

  // Result::Invariant and Trace members
  //
  using Invariant = PdrResult::Invariant;
  using Trace     = PdrResult::Trace;
  Invariant::Invariant(int l) : level(l) {}

  Trace::Trace() : length(0) {}
  Trace::Trace(unsigned l) : length(l) {}
  Trace::Trace(shared_ptr<const PdrState> s) : length(0)
  {
    while (s)
    {
      states.push_back(s->cube);
      length++;
      s = s->prev;
    }
  }

  unsigned Trace::total_pebbled() const
  {
    assert(!states.empty());
    unsigned pebbled = 0;
    for (z3::expr_vector const& s : states)
      pebbled = std::max(pebbled, state::no_marked(s));
    return pebbled;
    ;
  }

  // Result members
  //
  PdrResult::PdrResult(std::variant<Invariant, Trace> o) : output(o) {}

  PdrResult::PdrResult(std::shared_ptr<PdrState> s) : output(Trace(s)) {}

  PdrResult::PdrResult(int level) : output(Invariant(level)) {}

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

#warning TODO int level to size_t
  PdrResult PdrResult::found_invariant(int level) { return PdrResult(level); }

  PdrResult PdrResult::empty_true() { return PdrResult(-1); }
  PdrResult PdrResult::empty_false() { return PdrResult(nullptr); }

  void PdrResult::append_final(z3::expr_vector const& f)
  {
    assert(has_trace());
    trace().states.push_back(f);
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

  // IpdrResult
  // Public members
  //

  IpdrResult::IpdrResult(IModel const& m)
      : initial_state(m.get_initial()), vars(m.vars)
  {
  }

  IpdrResult::IpdrResult(z3::expr_vector I, mysat::primed::VarVec const& v)
      : initial_state(I), vars(v)
  {
  }

  IpdrResult::~IpdrResult() {}

  void IpdrResult::reset() { pdr_summaries.resize(0); }

  double IpdrResult::get_total_time() const { return total_time; }

  std::vector<double> IpdrResult::g_times() const
  {
    std::vector<double> times;
    std::transform(original.begin(), original.end(), std::back_inserter(times),
        [](PdrResult const& r) { return r.time; });
    return times;
  }

  tabulate::Table IpdrResult::summary_table() const
  {
    tabulate::Table t;
    t.add_row(summary_header());

    for (auto const& row : pdr_summaries)
      t.add_row(row);

    tabulate::Table::Row_t total;
    {
      total.resize(pdr_summaries.at(0).size()); // match no. columns

      std::vector<double> times = g_times();
      assert(total_time == std::accumulate(times.begin(), times.end(), 0.0));
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
    for (PdrResult const& result : original)
      ss << process_trace(result) << std::endl;
    return ss.str();
  }

  IpdrResult& IpdrResult::add(PdrResult const& r)
  {
    original.push_back(r);
    total_time += r.time;

    tabulate::Table::Row_t res_row = process_row(r);
    assert(res_row.size() == summary_header().size());
    pdr_summaries.push_back(res_row);

    return *this;
  }

  const tabulate::Table::Row_t IpdrResult::process_row(PdrResult const& r)
  {
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
    return result::trace_table(res, vars, initial_state);
  }

  const tabulate::Table::Row_t IpdrResult::summary_header() const
  {
    tabulate::Table::Row_t rv;
    rv.assign(PdrResult::fields.cbegin(), PdrResult::fields.cend());
    return rv;
  }

  // GENERAL
  //
  namespace result
  {
    using mysat::primed::VarVec;

    std::string trace_table(
        PdrResult const& res, VarVec const& vars, z3::expr_vector initial)
    {
      using fmt::format;
      using std::string;
      using std::to_string;
      using tabulate::Table;
      using z3::expr;
      using z3::expr_vector;

      if (res.has_invariant())
        return "Invariant, no trace.";

      // process trace
      std::stringstream ss;
      vector<string> lits  = vars.names();
      vector<string> litsp = vars.names_p();
      std::sort(lits.begin(), lits.end());
      std::sort(litsp.begin(), litsp.end());

      size_t longest =
          std::max_element(lits.begin(), lits.end(), str_size_cmp)->size();

      Table t;
      // Write top row
      {
        Table::Row_t trace_header = {
          "",
        };
        trace_header.insert(trace_header.end(), lits.begin(), lits.end());
        t.add_row(trace_header);
      }

      auto make_row =
          [longest](string i, const expr_vector& s, const vector<string>& names)
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
          expr_vector const& s = res.trace().states[i];
          string index_str;
          {
            if (i == 0)
            {
              assert(z3ext::quick_implies(s, initial)); // s \in I
              index_str = "I";
            }
            else if (i == N - 1)
              index_str = "(!P) " + to_string(i);
            else
              index_str = to_string(i);
          }

          Table::Row_t row_marking =
              make_row(index_str, s, (i < N - 1 ? lits : litsp));
          t.add_row(row_marking);
        }
      }

      t.format().font_align(tabulate::FontAlign::right);

      ss << tabulate::MarkdownExporter().dump(t);
      return ss.str();
    }
  } // namespace result
} // namespace pdr
