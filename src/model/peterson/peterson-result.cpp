#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include <tabulate/row.hpp>
#include <tabulate/table.hpp>

namespace pdr::peterson
{
  PetersonResult::PetersonResult(const PetersonModel& m, Tactic t)
      : IpdrResult(m), model(m), tactic(t)
  {
    assert(tactic == Tactic::decrement || tactic == Tactic::increment);
  }

  // PeterModel private members
  //
  // processes | max_processes | invariant | trace | time
  const tabulate::Table::Row_t PetersonResult::header() const
  {
    return { "processes", "max_processes", "invariant index", "trace_length",
      "time" };
  }
} // namespace pdr::peterson
