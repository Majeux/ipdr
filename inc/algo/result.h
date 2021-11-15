#ifndef PDR_RES
#define PDR_RES

#include "TextTable.h"
#include "obligation.h"
#include "output.h"
#include "pdr-model.h"
#include <fmt/format.h>
#include <memory>
#include <sstream>
#include <vector>
namespace pdr
{
  struct PDResult
  {
    std::shared_ptr<State> trace;
    std::string trace_string;
    unsigned trace_length;
    int pebbles_used;
    int invariant_index;
    double total_time;

    PDResult()
        : trace(nullptr), trace_string(""), trace_length(0), pebbles_used(-1),
          invariant_index(-1), total_time(0.0)
    {
    }

    std::vector<std::string> listing() const
    {
      return {std::to_string(pebbles_used), std::to_string(invariant_index),
              std::to_string(trace_length), std::to_string(total_time)};
    }
  };

  struct PDResults
  {
    const PDRModel& model;
    std::vector<PDResult> vec;

    PDResults(const PDRModel& m) : model(m), vec(1) {}

    PDResult& current() { return vec.back(); }
    void extend() { vec.emplace_back(); }

    void show(std::ostream& out) const
    {
      TextTable t;
      t.setAlignment(0, TextTable::Alignment::RIGHT);
      t.setAlignment(1, TextTable::Alignment::RIGHT);
      t.setAlignment(2, TextTable::Alignment::RIGHT);
      t.setAlignment(3, TextTable::Alignment::RIGHT);

      out << fmt::format("Pebbling strategies for {}:", model.name) << std::endl
          << std::endl;
      out << SEP2 << std::endl;

      std::vector<std::string> header = {"pebbles", "invariant index",
                                         "strategy length", "Total time"};
      t.addRow(header);
      for (const PDResult& res : vec)
        t.addRow(res.listing());

      out << t << std::endl << std::endl;

      for (const PDResult& res : vec)
      {
        if (res.trace)
        {
          out << fmt::format("Strategy for {} pebbles", res.pebbles_used)
              << std::endl;
          out << fmt::format("Target: [ {} ]", model.n_property.currents())
              << std::endl
              << std::endl;

          std::string line_form = "{:>{}} |\t [ {} ] No. pebbled = {}";
          std::string initial = z3ext::join_expr_vec(model.get_initial());
          std::string final = z3ext::join_expr_vec(model.n_property.currents());
          std::stringstream ss;
          unsigned padding = res.trace->show(ss);

          out << "Trace:" << std::endl
              << fmt::format(line_form, 'I', padding, initial, 0) << std::endl
              << ss.str()
              << fmt::format(line_form, 'F', padding, final,
                             model.get_f_pebbles())
              << std::endl;
        }
        else
        {
          out << fmt::format("No strategy for {} pebbles",
                             model.get_max_pebbles())
              << std::endl
              << std::endl;
        }
      }
    }
  };
} // namespace pdr
#endif // PDR_RES
