#ifndef PEBBLING_RESULT_H
#define PEBBLING_RESULT_H

#include "result.h"
#include "pebbling-model.h"

namespace pdr::pebbling 
{
    // aggregates multiple pdr runs into a single ipdr result for pebbling
    // collects: total time spent, highest level invariant, and trace with the
    // lowest marking
    class PebblingResult final : public IpdrResult
    {
     public:
      struct PebblingInvariant
      {
        PdrResult::Invariant invariant;
        std::optional<unsigned> constraint;
      };
      struct PebblingTrace
      {
        PdrResult::Trace trace;
        unsigned pebbled{ 0 };
      };
      struct Data_t
      {
        double time{ 0.0 };
        std::optional<PebblingInvariant> inv;
        std::optional<PebblingTrace> strategy;
      };

      PebblingResult(const pebbling::PebblingModel& m, Tactic t);
      PebblingResult(const IpdrResult& r, const PebblingModel& m, Tactic t);

      void add_to_table(tabulate::Table& t) const;
      void show_raw(std::ostream& out) const;

      void show(std::ostream& out) const override;
      // expand row with constraint and length, and store the latest in total
      tabulate::Table::Row_t table_row(const PdrResult& r) override;

      const Data_t& get_total() const;
      const std::optional<unsigned> min_pebbles() const;

     private:
      const PebblingModel& model;
      const Tactic tactic;
      Data_t total; // the latest invariant and trace, with the total time spent
      unsigned invariants{ 0 };
      unsigned traces{ 0 };

      const tabulate::Table::Row_t header() const override;
      std::string process_trace(const PdrResult& res) const override;
    };
} // namespace pdr::peterson

#endif // PEBBLING_RESULT_H
