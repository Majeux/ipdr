#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "string-ext.h"
#include "tabulate-ext.h"
#include "tactic.h"
#include "types-ext.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <fmt/ranges.h>
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
  using std::string;
  using std::vector;

  IpdrPetersonResult::IpdrPetersonResult(const PetersonModel& m, Tactic t)
      : IpdrResult(m), model(m), processes(m.n_processes()), tactic(t)
  {
    assert(tactic == Tactic::relax);
  }

  // PetersonModel public members
  //
  IpdrPetersonResult& IpdrPetersonResult::add(
      const PdrResult& r, unsigned n_switches)
  {
    tabulate::Table::Row_t res_row = process_result(r, n_switches);
    assert(res_row.size() == summary_header().size() - 1);
    pdr_summaries.push_back(res_row);

    return *this;
  }

  double IpdrPetersonResult::get_total_time() const { return total_time; }
  bool IpdrPetersonResult::all_holds() const { return holds; }

  std::string IpdrPetersonResult::end_result() const
  {
    return fmt::format("Peterson protocol proven up to {} context switches",
        last_proof_switches);
  }

  tabulate::Table::Row_t IpdrPetersonResult::total_row() const
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
    string proven_str = fmt::format("{}", proc_values);

    auto limit_str = pdr_summaries.front().at(1);
    // assert(std::all_of(pdr_summaries.cbegin(), pdr_summaries.cend(),
    //     [&limit_str](const Table::Row_t& row) -> bool
    //     { return to_string(row.at(1)) == to_string(limit_str); }));

    return { time_str, proven_str, limit_str };
  }

  // PetersonModel private members
  //
  // processes | max_processes | invariant | trace | time
  const tabulate::Table::Row_t IpdrPetersonResult::summary_header() const
  {
    auto rv = IpdrResult::summary_header();
    rv.insert(rv.begin(), "max switches");
    rv.insert(rv.begin(), "processes");
    return rv;
  }

  const tabulate::Table::Row_t IpdrPetersonResult::total_header() const
  {
    return peterson_total_header;
  }

  const tabulate::Table::Row_t IpdrPetersonResult::process_result(
      const PdrResult& r, unsigned n_switches)
  {
    // row with { invariant level, trace length, time }
    tabulate::Table::Row_t row = IpdrResult::process_result(r);
    // expand to { processes, max_proc, invariant level, trace length, time }
    if (r.has_trace())
    {
      holds = false;
      std::cout << process_trace(r) << std::endl;
    }
    else
      last_proof_switches = n_switches;

    row.insert(row.begin(), std::to_string(n_switches));
    row.insert(row.begin(), std::to_string(processes));

    assert(row.size() == summary_header().size() - 1); // inc time to be added

    return row;
  }

  std::string IpdrPetersonResult::process_trace(PdrResult const& res) const
  {
    return result::trace_table(res, vars, vars_p, model);
  }

  namespace result
  {
    std::string trace_table(PdrResult const& res,
        std::vector<std::string> vars,
        std::vector<std::string> vars_p,
        PetersonModel const& model)
    {
      using fmt::format;
      using std::to_string;
      using tabulate::Table;
      using z3::expr;
      using z3::expr_vector;
      using TraceState = PdrResult::Trace::TraceState;

      if (res.has_invariant())
      {
        return format("Peterson protocol correct for {} processes up to {} "
                      "context switches.\n",
            model.n_processes(),
            my::optional::to_string(model.get_switch_bound()));
      }

      std::sort(vars.begin(), vars.end());
      std::sort(vars_p.begin(), vars_p.end());

      // process trace
      std::stringstream ss;

      Table t, state_t;
      // Write top row
      {
        Table::Row_t trace_header = { "" };
        trace_header.insert(trace_header.end(), vars.begin(), vars.end());
        t.add_row(trace_header);
        state_t.add_row({ "", "" });
      }

      auto make_row =
          [](string a, TraceState const& s, const vector<string>& names)
      {
        std::vector<std::string> r = state::marking(s, names);
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

          t.add_row(make_row(index_str, s, (i < N - 1 ? vars : vars_p)));
          // also create readable peterson state representation
          {
            expr_vector current(model.ctx), next(model.ctx);
            for (z3ext::LitStr const& l : s)
            {
              expr c = model.ctx.bool_const(l.atom.c_str());
              expr n = model.vars.p(c);
              if (l.value)
              {
                current.push_back(c);
                next.push_back(n);
              }
              else
              {
                current.push_back(!c);
                next.push_back(!n);
              }
            }

            string state_str = i < N - 1
                                 ? model.extract_state(current).to_string(true)
                                 : model.extract_state_p(next).to_string(true);
            state_t.add_row({ index_str, state_str });
          }
        }
        ss << format("Trace to mutex violation within {} context switches",
                  my::optional::to_string(model.get_switch_bound()))
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
  } // namespace result
} // namespace pdr::peterson
