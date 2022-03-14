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
  struct Result
  {
    std::shared_ptr<State> trace;
    std::string trace_string;
    unsigned trace_length;
    int pebbles_used;
    int invariant_index;
    double total_time;

    Result()
        : trace(nullptr), trace_string(""), trace_length(0), pebbles_used(-1),
          invariant_index(-1), total_time(0.0)
    {
    }

    std::vector<std::string> listing() const
    {
      return { std::to_string(pebbles_used), std::to_string(invariant_index),
               std::to_string(trace_length), std::to_string(total_time) };
    }
  };

  struct Results
  {
    const Model& model;
    std::vector<Result> vec;

    Results(const Model& m) : model(m), vec(1) {}

    Result& current() { return vec.back(); }
    void extend() { vec.emplace_back(); }

    void show(std::ostream& out) const
    {
      TextTable t;

      out << fmt::format("Pebbling strategies for {}:", model.name) << std::endl
          << std::endl;
      out << SEP2 << std::endl;

      std::vector<std::string> header = { "pebbles", "invariant index",
                                          "strategy length", "Total time" };
      for (unsigned i = 0; i < header.size(); i++)
        t.setAlignment(i, TextTable::Alignment::RIGHT);

      t.addRow(header);
      for (const Result& res : vec)
        t.addRow(res.listing());

      out << t << std::endl << std::endl;

      for (const Result& res : vec)
        out << res.trace_string << std::endl;
    }
  };
} // namespace pdr
#endif // PDR_RES
