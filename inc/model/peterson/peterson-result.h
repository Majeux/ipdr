#ifndef PETER_RESULT_H
#define PETER_RESULT_H

#include "peterson.h"
#include "result.h"

namespace pdr::peterson
{
  // processes | max_processes | invariant | trace | time
  class PetersonResult final : public IpdrResult
  {
   public:
    PetersonResult(const PetersonModel& m, Tactic t);

   private:
    const PetersonModel& model;
    const Tactic tactic;
    unsigned max_processes;
    unsigned processes;
    unsigned invariants{ 0 };
    unsigned traces{ 0 };

    const tabulate::Table::Row_t header() const override;
    std::string process_trace(const PdrResult& res) const override;
  }; // class PeterState
} // namespace pdr::peterson

#endif // PETER_RESULT_H
