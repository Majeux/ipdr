#ifndef Z3PDR_H
#define Z3PDR_H

#include "dag.h"
#include "expr.h"
#include "logger.h"
#include "pdr-context.h"
#include "pebbling-result.h"
#include "result.h"
#include "vpdr.h"
#include "z3-pebbling-model.h"

#include <z3++.h>

namespace pdr::test
{
  class z3PebblingIPDR;

  class z3PDR : public vPDR
  {
    friend class z3PebblingIPDR;

   public:
    z3PDR(Context c, Logger& l, Z3Model& m);

    PdrResult run() override;
    void reset() override;
    void show_solver(std::ostream& out) const override;

   private:
    struct Rule
    {
      z3::expr expr;
      z3::symbol name;

      Rule(z3::context& ctx) : expr(ctx), name(ctx.str_symbol("empty str")) {}
      Rule(z3::expr const& e, z3::symbol const& n) : expr(e), name(n) {}
    };

    Z3Model& ts;
    z3::fixedpoint engine;
    z3::check_result last_result = z3::check_result::unknown;

    std::vector<std::string> get_trace();
    std::vector<std::string> get_trace_states();
  };

  // class to verify against z3's pdr implementation
  // runs pdr multiple times, but without incremental optimizations
  class z3PebblingIPDR
  {
   public:
    z3PebblingIPDR(Context& c, Z3PebblingModel& m,
        my::cli::ArgumentList const& args, Logger& l);

    // run ipdr without any incremental funtionality
    pebbling::PebblingResult control_run(Tactic tactic);
    pebbling::PebblingResult relax(bool control);
    pebbling::PebblingResult constrain(bool control);

    pebbling::PebblingResult new_total(Tactic t) const;
    z3PDR const& internal_alg() const;

   private:
    z3PDR alg;
    Z3PebblingModel& ts;
    std::optional<unsigned> starting_pebbles;

    void basic_reset(unsigned pebbles);
  }; // class Optimizer
} // namespace pdr::test

#endif // Z3PDR_H
