#ifndef VPDR_H
#define VPDR_H

#include "logger.h"
#include "pdr-context.h"
#include "result.h"
#include <ostream>

namespace pdr
{
  class vPDR
  {
   public:
    Context ctx;
    Logger& logger;

    vPDR(Context c, Logger& l) : ctx(c), logger(l) {}
    virtual ~vPDR() {}

    Context const& get_ctx() const { return ctx; };
    // Context& get_ctx() { return ctx; }

    // run pdr, returning either an inductive invariant or a counterexample
    // trace
    virtual PdrResult run() = 0;

    // rest pdr's internal state. discards any recorded information or state
    virtual void reset() = 0;

    virtual void show_solver(std::ostream& out) const = 0;
  };
} // namespace pdr

#endif // VPDR_H
