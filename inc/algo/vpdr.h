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

    // reset pdr's internal state. discards any recorded information or state
    virtual void reset() = 0;
    // reset pdr's internal state. if supported, copy information as per the
    // constraining ipdr algorithm.
    // @return: if constraining detects an inductive invariant, return its level
    virtual std::optional<size_t> constrain() = 0;
    // reset pdr's internal state. if supported, copy information as per the
    // relaxing ipdr algorithm
    virtual void relax() = 0;

    virtual void show_solver(std::ostream& out) const = 0;
  };
} // namespace pdr

#endif // VPDR_H
