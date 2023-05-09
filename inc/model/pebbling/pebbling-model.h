#ifndef PEBBLING_MODEL
#define PEBBLING_MODEL

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <z3++.h>

#include "cli-parse.h"
#include "dag.h"
#include "expr.h"
#include "pdr-model.h"

namespace pdr::pebbling
{
  class PebblingModel : public pdr::IModel
  {
   public:
    const dag::Graph dag;
    // std::string name;
    // z3::context ctx;
    // ExpressionCache lits;
    // ExpressionCache property;
    // ExpressionCache n_property;

    // const z3::expr_vector& get_transition() const;
    // const z3::expr_vector& get_initial() const;

    // void show(std::ostream& out) const;

    PebblingModel(
        const my::cli::ArgumentList& args, z3::context& c, const dag::Graph& G);
    PebblingModel& constrained(std::optional<unsigned> maximum_pebbles);

    // set a constraint on the transition relation to reduce the state-space
    void constrain(std::optional<unsigned> new_p);

    size_t n_nodes() const;
    // return the number of pebbles in the final state
    unsigned get_f_pebbles() const;
    // return the current maximum number of pebbles
    std::optional<unsigned> get_pebble_constraint() const;
    // return string representation of the constraint
    const std::string constraint_str() const override;
    // number representative of the constraint. a larger number is a looser
    // constraint
    unsigned constraint_num() const override;

   private:
    // z3::expr_vector initial;
    // z3::expr_vector transition; // vector of clauses (cnf)
    // z3::expr_vector constraint; // vector of clauses (cnf)

    // number of marked literals in property
    unsigned final_pebbles;
    // maximum number of pebbled nodes allowed per state
    std::optional<unsigned> pebble_constraint;

    // cnf formula: expanded the original implication into conjunction of clauses
    void load_pebble_transition(const dag::Graph& G);
    void load_pebble_transition_tseytin_custom(const dag::Graph& G);
    void load_pebble_transition_z3tseytin(const dag::Graph& G);
    // non-cnf formula: one implication for each child
    void load_pebble_transition_raw1(const dag::Graph& G);
    // non-cnf formula: one implication per parent
    void load_pebble_transition_raw2(const dag::Graph& G);
    void load_property(const dag::Graph& G);
  };
} // namespace pdr::pebbling

#endif // !PEBBLING_MODEL
