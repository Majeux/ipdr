#ifndef PDR_MODEL
#define PDR_MODEL

#include <memory>
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
        const dag::Graph& G, int pebbles);
    void load_model(
        const std::string& model_name, const dag::Graph& G, int max_pebbles);
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_initial() const;
    const z3::expr_vector& get_cardinality() const;
    size_t n_nodes() const;
    int get_max_pebbles() const;
    // sets the new constraint, returns false if final state cannot be pebbled
    void set_max_pebbles(int x);
    int get_f_pebbles() const;
    void show(std::ostream& out) const;

   private:
    int max_pebbles;
    int final_pebbles;

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
