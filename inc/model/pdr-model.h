#ifndef PDR_MODEL
#define PDR_MODEL

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <z3++.h>

#include "dag.h"
#include "exp-cache.h"

namespace pdr
{
  class PebblingModel
  {
   public:
    std::string name;
    z3::context ctx;
    ExpressionCache lits;
    ExpressionCache property;
    ExpressionCache n_property;

    PebblingModel(z3::config& settings, const std::string& model_name,
        const dag::Graph& G, std::optional<unsigned> pebbles);
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_initial() const;
    const z3::expr_vector& get_cardinality() const;
    size_t n_nodes() const;
    // the maximal amount of pebbles allowed
    unsigned get_constraint() const;
    // set the maximal amount of pebbles allowed
    void set_constaint(unsigned x);
    // return the number of pebbles in the final state
    int get_f_pebbles() const;
    void show(std::ostream& out) const;

   private:
    unsigned max_pebbles;
    unsigned final_pebbles;

    z3::expr_vector initial;
    z3::expr_vector transition; // vector of clauses (cnf)
    // cardinality constraint for current and next state
    z3::expr_vector cardinality;

    z3::config& set_config(z3::config& settings);
    void load_pebble_transition(const dag::Graph& G);
    void load_pebble_transition_tseytin(const dag::Graph& G);
    void load_pebble_transition_raw1(const dag::Graph& G);
    void load_pebble_transition_raw2(const dag::Graph& G);
    void load_property(const dag::Graph& G);

    z3::expr tseytin_and(z3::expr_vector& sub_defs, const std::string& name,
        const z3::expr& a, const z3::expr& b);
    z3::expr tseytin_or(z3::expr_vector& sub_defs, const std::string& name,
        const z3::expr& a, const z3::expr& b);
    z3::expr tseytin_implies(z3::expr_vector& sub_defs, const std::string& name,
        const z3::expr& a, const z3::expr& b);
    z3::expr tseytin_xor(z3::expr_vector& sub_defs, const std::string& name,
        const z3::expr& a, const z3::expr& b);
  };
} // namespace pdr

#endif // !PDR_MODEL
