#ifndef Z3PDR_H
#define Z3PDR_H

#include "dag.h"
#include "expr.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "result.h"

#include <z3++.h>

namespace pdr::test
{
  class z3PDR
  {
   public:
    z3PDR(Context& m, Logger& l, dag::Graph const& G);

    void run();

   private:
    struct Rule
    {
      z3::expr expr;
      z3::symbol name;

      Rule(z3::context& ctx) : expr(ctx), name(ctx.str_symbol("empty str")) {}
      Rule(z3::expr const& e, z3::symbol const& n) : expr(e), name(n) {}
    };

    Context& ctx;
    Logger& log;
    z3::fixedpoint engine;

    z3::expr const z3_true  = ctx().bool_val(true);
    z3::expr const z3_false = ctx().bool_val(false);

    mysat::primed::VarVec vars;

    std::vector<Rule> rules;
    z3::expr target;
    // z3::func_decl target_decl;

    z3::sort_vector state_sorts;
    z3::func_decl state; // B^N |-> B
    z3::func_decl step;  // B^N B^N |-> B

    Rule reach_rule;
    Rule initial;

    Rule mk_rule(z3::expr const& e, std::string const& n);
    Rule mk_rule(
        z3::expr const& head, z3::expr const& body, std::string const& n);
    z3::expr forall_vars(z3::expr const& e) const;

    Rule pebbling_transition(z3::expr const& parent,
        std::set<z3::expr, z3ext::expr_less> const& children);

    std::vector<std::string> get_trace();
  };
} // namespace pdr::test

#endif // Z3PDR_H
