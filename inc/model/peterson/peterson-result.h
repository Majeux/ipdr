#ifndef PETER_RESULT_H
#define PETER_RESULT_H

#include "peterson.h"
#include "result.h"

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

    void show(std::ostream& out) const override;
    void add_to_table(tabulate::Table& t) const;

   private:
    const PetersonModel& model;
    const Tactic tactic;
    unsigned max_processes;
    unsigned processes;
    unsigned invariants{ 0 };
    unsigned traces{ 0 };
    double total_time{ 0.0 };

    const tabulate::Table::Row_t header() const override;
    const tabulate::Table::Row_t table_row(const PdrResult& r) override;
    std::string process_trace(const PdrResult& res) const override;
  }; // class PeterState
} // namespace pdr::peterson

#endif // PETER_RESULT_H
