#ifndef PEBBLING_RESULT_H
#define PEBBLING_RESULT_H

#include "expr.h"
#include "pebbling-model.h"
#include "result.h"
#include <tabulate/table.hpp>
#include <z3++.h>

namespace pdr::pebbling
{
  // aggregates multiple pdr runs into a single ipdr result for pebbling
  // collects: total time spent, highest level invariant, and trace with the
  // lowest marking
  class PebblingResult final : public IpdrResult
  {
   public:
    inline static const tabulate::Table::Row_t pebbling_summary_header = {
      "constraint", "pebbled", "invariant index", "trace length", "time"
    };
    inline static const tabulate::Table::Row_t pebbling_total_header = {
      "runtime", "min constraint strategy", "length"
    };

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
      double const& time; // refers to IpdrResult::total_time
      std::optional<PebblingInvariant> inv;
      std::optional<PebblingTrace> strategy;
    };

    // construct PebblingResult
    PebblingResult(const PebblingModel& m, Tactic t);
    PebblingResult(z3::expr_vector initial, mysat::primed::VarVec const& vars,
        unsigned pebbles_final, std::optional<unsigned> pebbles_max, Tactic t);
    // convert from general IpdrResult to PebblingResult
    PebblingResult(const IpdrResult& r, const PebblingModel& m, Tactic t);
    PebblingResult(const IpdrResult& r, unsigned pebbles_final,
        std::optional<unsigned> pebbles_max, Tactic t);

    Data_t const& get_total() const;
    std::string end_result() const override;
    const std::optional<unsigned> min_pebbles() const;
    tabulate::Table::Row_t total_row() const override;

   private:
    // pebbling model info
    unsigned pebbles_final;
    std::optional<unsigned> pebble_constraint;

    const Tactic tactic;
    // the latest invariant and trace, with the total time spent
    // if constraining: strategy = the latest of the multiple strategies
    //                  inv = the first invariant found
    // if relaxing:     strategy = the only (first) strategy found
    //                  inv = the highest invariant found
    Data_t total;
    unsigned n_invariants{ 0 };
    unsigned n_traces{ 0 };

    const tabulate::Table::Row_t summary_header() const override;
    const tabulate::Table::Row_t total_header() const override;
    // expand row with constraint and length, and store the latest in total
    const tabulate::Table::Row_t process_row(const PdrResult& r) override;
    std::string process_trace(const PdrResult& res) const override;
  };
} // namespace pdr::pebbling

#endif // PEBBLING_RESULT_H
