#include "result.h"
#include "obligation.h"
#include "output.h"
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
    for (const z3::expr_vector& s : states)
      pebbled = std::max(pebbled, state::no_marked(s));
    return pebbled;;
  }

  // Result members
  //
  PdrResult::PdrResult(std::shared_ptr<PdrState> s) : output(Trace(s)) {}

  PdrResult::PdrResult(int l) : output(Invariant(l)) {}

  PdrResult PdrResult::found_trace(std::shared_ptr<PdrState> s)
  {
    return PdrResult(s);
  }

  PdrResult PdrResult::found_trace(PdrState&& s)
  {
    return PdrResult(std::make_shared<PdrState>(s));
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

    if (has_invariant())
      return { format("F_{}", invariant().level), "", to_string(time) };
    else
      return { "", to_string(trace().length), to_string(time) };
  }

  // IpdrResult
  // Public members
  //

  IpdrResult::IpdrResult(const IModel& m) : model(m) {}
  IpdrResult::~IpdrResult() {}

  tabulate::Table IpdrResult::new_table() const
  {
    tabulate::Table t;
    t.add_row(header());
    return t;
  }

  void IpdrResult::reset() { rows.resize(0); }

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
    for (const PdrResult& result : original)
      out << process_trace(result) << std::endl;
  }

  void IpdrResult::show(std::ostream& out) const
  {
    tabulate::Table t = raw_table();
    out << t << std::endl << std::endl;

    show_traces(out);
  }

  IpdrResult& IpdrResult::add(const PdrResult& r)
  {
    original.push_back(r);

    tabulate::Table::Row_t res_row = table_row(r);
    assert(res_row.size() == header().size());
    rows.push_back(res_row);

    return *this;
  }

  const tabulate::Table::Row_t IpdrResult::table_row(const PdrResult& r)
  {
    tabulate::Table::Row_t row;

    PdrResult::ResultRow listing = r.listing();
    row.assign(listing.begin(), listing.end());

    return row;
  }

  IpdrResult& operator<<(IpdrResult& rs, const PdrResult& r)
  {
    return rs.add(r);
  }

  // Private members
  //
  std::string IpdrResult::process_trace(const PdrResult& res) const
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
    std::vector<std::string> lits = model.vars.names();
    std::sort(lits.begin(), lits.end());

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

    auto make_row = [&lits, longest](string i, const PdrState& s)
    {
      std::vector<std::string> r = state::marking(s, lits, longest);
      r.insert(r.begin(), string(i));
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write initial state
    {
      expr_vector initial_state = model.get_initial();
      Table::Row_t initial_row  = make_row("0", PdrState(initial_state));
      t.add_row(initial_row);
    }
    // Write trace states
    {
      for (size_t i = 0; i < res.trace().states.size(); i++)
      {
        const PdrState& s           = res.trace().states[i];
        Table::Row_t row_marking = make_row(to_string(i), s);
        t.add_row(row_marking);
      }
    }
    // Write final state
    {
      Table::Row_t final_row = make_row("!P", PdrState(model.n_property));
      t.add_row(final_row);
    }

    t.format().font_align(tabulate::FontAlign::right);

    ss << tabulate::MarkdownExporter().dump(t);
    return ss.str();
  }

  const tabulate::Table::Row_t IpdrResult::header() const
  {
    return { "invariant index", "trace length", "time" };
  }

  const tabulate::Table::Row_t IpdrResult::summary_header() const
  {
    tabulate::Table::Row_t rv;
    rv.assign(PdrResult::header.cbegin(), PdrResult::header.cend());
    return rv;
  }
} // namespace pdr
