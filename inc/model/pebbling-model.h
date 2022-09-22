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
    // std::string name;
    // z3::context ctx;
    // ExpressionCache lits;
    // ExpressionCache property;
    // ExpressionCache n_property;

    // const z3::expr_vector& get_transition() const;
    // const z3::expr_vector& get_initial() const;

    // void show(std::ostream& out) const;

    PebblingModel(z3::context& c, const my::cli::ArgumentList& model_name,
        const dag::Graph& G);

    // set a constraint on the transition relation to reduce the state-space
    void constrain(std::optional<unsigned> new_p);

    size_t n_nodes() const;
    // return the number of pebbles in the final state
    unsigned get_f_pebbles() const;
    // return the current maximum number of pebbles
    std::optional<unsigned> get_max_pebbles() const;

   private:
    // z3::expr_vector initial;
    // z3::expr_vector transition; // vector of clauses (cnf)

    unsigned final_pebbles; // number of marked literals in property
    std::optional<unsigned> max_pebbles;

    void load_pebble_transition(const dag::Graph& G);
    void load_pebble_transition_tseytin(const dag::Graph& G);
    void load_pebble_transition_raw1(const dag::Graph& G);
    void load_pebble_transition_raw2(const dag::Graph& G);
    void load_property(const dag::Graph& G);
  };
} // namespace pdr::pebbling

#endif // !PEBBLING_MODEL
