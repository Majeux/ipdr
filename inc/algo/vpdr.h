#ifndef VPDR_H
#define VPDR_H

#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "result.h"
#include <ostream>

namespace pdr
{
  class vPDR
  {
   public:
    Context ctx;
    Logger& logger;

    vPDR(Context c, Logger& l, IModel& m) : ctx(c), logger(l), ts(m) {}
    virtual ~vPDR() {}

    Context const& get_ctx() const { return ctx; };
    // Context& get_ctx() { return ctx; }

    // run pdr, returning either an inductive invariant or a counterexample
    // trace
    virtual PdrResult run() = 0;

    // reset pdr's internal state. discards any recorded information or state
    virtual void reset()                      = 0;
    // reset pdr's internal state. if supported, copy information as per the
    // constraining ipdr algorithm.
    // @return: if constraining detects an inductive invariant, return its level
    virtual std::optional<size_t> constrain() = 0;
    // reset pdr's internal state. if supported, copy information as per the
    // relaxing ipdr algorithm
    virtual void relax()                      = 0;

    virtual void show_solver(std::ostream& out) const = 0;

   protected:
    IModel& ts;

    // logging shorthands
    void log_start() const;
    void log_iteration(size_t frame);
    void log_cti(std::vector<z3::expr> const& cti, unsigned level);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(size_t queue_size,
        unsigned top_level,
        std::vector<z3::expr> const& top);
    void log_pred(std::vector<z3::expr> const& p);
    void log_state_push(unsigned frame);
    void log_finish_state(std::vector<z3::expr> const& s);
    void log_obligation_done(std::string_view type, unsigned l, double time);
    void log_pdr_finish(PdrResult const& r, double final_time);
  };
} // namespace pdr

#endif // VPDR_H
