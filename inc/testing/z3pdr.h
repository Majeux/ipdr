#ifndef Z3PDR_H
#define Z3PDR_H

#include "dag.h"
#include "expr.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "pebbling-result.h"
#include "result.h"
#include "vpdr.h"

#include <spdlog/stopwatch.h>
#include <tabulate/table.hpp>
#include <z3++.h>

namespace pdr::test
{
  class z3PebblingIPDR;

  class z3PDR : public vPDR
  {
    friend class z3PebblingIPDR;

   public:
    z3PDR(Context c, Logger& l, IModel& m);

    PdrResult run() override;
    void reset() override;
    std::optional<size_t> constrain() override;
    void relax() override;
    void show_solver(std::ostream& out) const override;

   private:
    z3::check_result last_result = z3::check_result::unknown;
    std::string cover_string{ "" };

    spdlog::stopwatch timer;


    std::vector<std::string> get_trace(z3::fixedpoint& engine);
    PdrResult::Trace::TraceVec get_trace_states(z3::fixedpoint& engine);

    z3::fixedpoint mk_prepare_fixedpoint();

  };
} // namespace pdr::test

#endif // Z3PDR_H
