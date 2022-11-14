#ifndef PETER_RESULT_H
#define PETER_RESULT_H

#include "peterson.h"
#include "result.h"

#include <optional>
#include <ostream>
#include <tabulate/tabulate.hpp>

namespace pdr::peterson
{
  namespace result
  {
    static tabulate::Table::Row_t result_header = { "processes",
      "max_processes", "invariant index", "trace_length", "time" };

    static tabulate::Table::Row_t summary_header = { "runtime",
      "proven for p=", "maximum p" };
  } // namespace result

  // processes | max_processes | invariant | trace | time
  class PetersonResult final : public IpdrResult
  {
   public:
    PetersonResult(const PetersonModel& m, Tactic t);

    double get_total_time() const;
    bool all_holds() const;

    void show(std::ostream& out) const override;
    void add_summary_to(tabulate::Table& t) const override;

   private:
    const PetersonModel& model;
    const Tactic tactic;
    unsigned max_processes;
    unsigned processes;
    bool holds{ true };

    const tabulate::Table::Row_t header() const override;
    const tabulate::Table::Row_t table_row(const PdrResult& r) override;
    std::string process_trace(const PdrResult& res) const override;
  }; // class PeterState
} // namespace pdr::peterson

#endif // PETER_RESULT_H
