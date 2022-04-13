#ifndef PDR_MODEL
#define PDR_MODEL

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <z3++.h>

#include "cli-parse.h"
#include "dag.h"
#include "exp-cache.h"

namespace pdr
{
  namespace pebbling
  {
    class Model
    {
     public:
      std::string name;
      z3::context ctx;
      ExpressionCache lits;
      ExpressionCache property;
      ExpressionCache n_property;

      Model(z3::config& settings, const my::cli::ArgumentList& model_name,
          const dag::Graph& G);
      const z3::expr_vector& get_transition() const;
      const z3::expr_vector& get_initial() const;
      size_t n_nodes() const;
      // return cardinality 'x' clauses for current and next literals
      z3::expr_vector constraint(std::optional<unsigned> x);
      // return the number of pebbles in the final state
      unsigned get_f_pebbles() const;
      void show(std::ostream& out) const;

     private:
      unsigned final_pebbles; // number of marked literals in property

      z3::expr_vector initial;
      z3::expr_vector transition; // vector of clauses (cnf)

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
      z3::expr tseytin_implies(z3::expr_vector& sub_defs,
          const std::string& name, const z3::expr& a, const z3::expr& b);
      z3::expr tseytin_xor(z3::expr_vector& sub_defs, const std::string& name,
          const z3::expr& a, const z3::expr& b);
    };
  } // namespace pebbling
} // namespace pdr

#endif // !PDR_MODEL
