#ifndef Z3_PEBBLING_MODEL_H
#define Z3_PEBBLING_MODEL_H

#include "cli-parse.h"
#include "dag.h"
#include "pdr-model.h"

namespace pdr::test
{
  class Z3PebblingModel : public Z3Model
  {
   public:
    const dag::Graph dag;

    Z3PebblingModel(
        const my::cli::ArgumentList& args, z3::context& c, const dag::Graph& G);
    Z3PebblingModel& constrained(std::optional<unsigned> maximum_pebbles);

    void add_initial(z3::fixedpoint& engine) override;
    void add_transitions(z3::fixedpoint& engine) override;
    z3::expr get_target() const override;
    z3::check_result reach_target(z3::fixedpoint& engine) override;

    void constrain(std::optional<unsigned> maximum_pebbles);
    z3::expr constraint_assertion();

    std::string to_string() const override;
    z3::expr_vector get_initial() const;
    std::optional<unsigned> get_pebble_constraint() const;

   private:
    std::optional<unsigned> pebble_constraint;

    z3::expr const z3_true  = ctx.bool_val(true);
    z3::expr const z3_false = ctx.bool_val(false);

    // the engine takes references to rules, so they must be stored
    std::vector<Rule> rules;
    Rule initial;
    z3::expr target;

    Rule reach_rule;
    z3::sort_vector state_sorts;
    z3::func_decl state; // B^N |-> B
    z3::func_decl step;  // B^N B^N |-> B

    Rule pebbling_transition(z3::expr const& parent,
        std::set<z3::expr, z3ext::expr_less> const& children);

    z3::expr make_constraint();

    void prepare_initial();
    void prepare_transitions();
    void prepare_target();
  };
} // namespace pdr::test

#endif // Z3_PEBBLING_MODEL_H
