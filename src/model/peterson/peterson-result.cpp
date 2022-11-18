#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "string-ext.h"
#include "tabulate-ext.h"
#include "tactic.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <fmt/core.h>
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
    assert(tactic == Tactic::constrain || tactic == Tactic::relax);
  }

  // PetersonModel public members
  //
  double PetersonResult::get_total_time() const { return total_time; }
  bool PetersonResult::all_holds() const { return holds; }

  std::string PetersonResult::end_result() const
  {
    assert(!all_holds() || last_proof_procs == model.max_processes());
    return fmt::format("Peterson protocol proven for {}..{} processes",
        model.n_processes(), last_proof_procs);
  }

  tabulate::Table::Row_t PetersonResult::total_row() const
  {
    using fmt::format;
    using std::string;
    using std::to_string;
    using tabulate::Table;
    using tabulate::to_string;

    string time_str = to_string(total_time);

    std::vector<string_view> proc_values;
    std::transform(pdr_summaries.cbegin(), pdr_summaries.cend(),
        std::back_inserter(proc_values),
        [](const Table::Row_t& r) -> string { return to_string(r.front()); });
    string proven_str = str::ext::join(proc_values);

    auto limit_str = pdr_summaries.front().at(1);
    assert(std::all_of(pdr_summaries.cbegin(), pdr_summaries.cend(),
        [&limit_str](const Table::Row_t& row) -> bool
        { return to_string(row.at(1)) == to_string(limit_str); }));

    return { time_str, proven_str, limit_str };
  }

  // PetersonModel private members
  //
  // processes | max_processes | invariant | trace | time
  const tabulate::Table::Row_t PetersonResult::summary_header() const
  {
    return { "processes", "max_processes", "invariant index", "trace_length",
      "time" };
  }

  const tabulate::Table::Row_t PetersonResult::total_header() const
  {
    return { "runtime", "proven for p=", "maximum p" };
  }

  const tabulate::Table::Row_t PetersonResult::process_row(const PdrResult& r)
  {
    // row with { invariant level, trace length, time }
    tabulate::Table::Row_t row = IpdrResult::process_row(r);
    // expand to { processes, max_proc, invariant level, trace length, time }
    if (r.has_trace())
    {
      holds = false;
      std::cout << process_trace(r) << std::endl;
    }
    else
      last_proof_procs = model.n_processes();

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
    using std::vector;
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
    vector<string> lits  = model.vars.names();
    vector<string> litsp = model.vars.names_p();
    std::sort(lits.begin(), lits.end());

    size_t longest =
        std::max_element(lits.begin(), lits.end(), str::ext::size_lt)->size();

    Table t, state_t;
    // Write top row
    {
      Table::Row_t trace_header = { "" };
      trace_header.insert(trace_header.end(), lits.begin(), lits.end());
      t.add_row(trace_header);
      state_t.add_row({ "", "" });
    }

    auto make_row =
        [&longest](string a, const expr_vector& s, const vector<string>& names)
    {
      std::vector<std::string> r = state::marking(s, names, longest);
      r.insert(r.begin(), a);
      Table::Row_t rv;
      rv.assign(r.begin(), r.end());
      return rv;
    };

    // Write strategy states
    {
      size_t N = res.trace().states.size();
      for (size_t i = 0; i < N; i++)
      {
        const z3::expr_vector& s = res.trace().states[i];

        string index_str = (i == 0) ? "I" : to_string(i);

        Table::Row_t row_marking =
            make_row(index_str, s, (i < N - 1 ? lits : litsp));
        t.add_row(row_marking);
        {
          const PetersonModel& m = dynamic_cast<const PetersonModel&>(model);
          string state_str = i < N - 1 ? m.extract_state(s).to_string(true)
                                       : m.extract_state_p(s).to_string(true);
          state_t.add_row({ index_str, state_str });
        }
      }
      ss << format("Trace to two processes with level[p] = N-1 = {}",
                model.max_processes() - 1)
         << std::endl
         << std::endl;
    }

    {
      tabulate::MarkdownExporter exp;
      t.format().font_align(tabulate::FontAlign::right);
      state_t.format().font_align(tabulate::FontAlign::right);

      ss << exp.dump(t) << std::endl
         << string(15, '=') << std::endl
         << exp.dump(state_t);
    }

    return ss.str();
  }
} // namespace pdr::peterson
