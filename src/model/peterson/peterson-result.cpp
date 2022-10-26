#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "string-ext.h"
#include "tabulate-ext.h"
#include "tactic.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <string>
#include <string_view>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/row.hpp>
#include <tabulate/table.hpp>
#include <vector>
#include <z3++.h>

namespace pdr::peterson
{
  PetersonResult::PetersonResult(const PetersonModel& m, Tactic t)
      : IpdrResult(m), model(m), tactic(t)
  {
    assert(tactic == Tactic::decrement || tactic == Tactic::increment);
  }

  // PetersonModel public members
  //
  double PetersonResult::get_total_time() const { return total_time; }
  bool PetersonResult::all_holds() const { return holds; }

  void PetersonResult::show(std::ostream& out) const
  {
    using fmt::format;
    using std::to_string;

    tabulate::Table table;
    table.format().font_align(tabulate::FontAlign::right);

    table.add_row({ "runtime", "proven for p=", "maximum p" });
    add_summary_to(table);

    out << table << std::endl;
    auto latex = tabulate::LatexExporter().dump(table);
    out << latex << std::endl;
  }

  void PetersonResult::add_summary_to(tabulate::Table& t) const
  {
    using fmt::format;
    using std::string;
    using std::to_string;
    using tabulate::Table;
    using tabulate::to_string;

    string time_str = to_string(total_time);

    std::vector<string_view> proc_values;
    std::transform(rows.cbegin(), rows.cend(), std::back_inserter(proc_values),
        [](const Table::Row_t& r) -> string { return to_string(r.front()); });
    string proven_str = str::ext::join(proc_values);

    auto limit_str = rows.front().at(1);
    assert(std::all_of(rows.cbegin(), rows.cend(),
        [&limit_str](const Table::Row_t& row) -> bool
        { return to_string(row.at(1)) == to_string(limit_str); }));

    t.add_row({ time_str, proven_str, limit_str });
  }

  // PetersonModel private members
  //
  // processes | max_processes | invariant | trace | time
  const tabulate::Table::Row_t PetersonResult::header() const
  {
    return result::result_header;
  }

  const tabulate::Table::Row_t PetersonResult::table_row(const PdrResult& r)
  {
    total_time += r.time;

    // row with { invariant level, trace length, time }
    tabulate::Table::Row_t row = IpdrResult::table_row(r);
    // expand to { processes, max_proc, invariant level, trace length, time }
    if (r.has_trace())
    {
      holds = false;
      std::cout << process_trace(r) << std::endl;
    }

    row.insert(row.begin(), std::to_string(model.max_processes()));
    row.insert(row.begin(), std::to_string(model.n_processes()));

    return row;
  }

  std::string PetersonResult::process_trace(const PdrResult& res) const
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
      return format("Peterson protocol correct for {} processes (out of {}).\n",
          model.n_processes(), model.max_processes());
    }

    // process trace
    std::stringstream ss;
    std::vector<std::string> lits = model.vars.names();
    std::sort(lits.begin(), lits.end());

    size_t longest =
        std::max_element(lits.begin(), lits.end(), str::ext::size_lt)->size();

    Table t;
    // Write top row
    {
      Table::Row_t trace_header = { "" };
      trace_header.insert(trace_header.end(), lits.begin(), lits.end());
      t.add_row(trace_header);
    }

    auto make_row = [&lits, longest](string a, const expr_vector& s)
    {
      std::vector<std::string> r = state::marking(s, lits, longest);
      r.insert(r.begin(), a);
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write strategy states
    {
      for (size_t i = 0; i < res.trace().states.size(); i++)
      {
        const z3::expr_vector& s = res.trace().states[i];

        string index_str = (i == 0) ? "I" : to_string(i);

        Table::Row_t row_marking = make_row(index_str, s);
        t.add_row(row_marking);
      }
      ss << format("Trace to two processes with level[p] = N-1 = {}",
                model.max_processes() - 1)
         << std::endl
         << std::endl;
    }

    t.format().font_align(tabulate::FontAlign::right);

    ss << tabulate::MarkdownExporter().dump(t);
    return ss.str();
  }
} // namespace pdr::peterson
