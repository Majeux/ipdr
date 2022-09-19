#ifndef PDR_MODEL
#define PDR_MODEL

#include <optional>
#include <z3++.h>

#include "expr.h"

namespace pdr
{
  class IModel
  {
   protected:
    z3::context& ctx;

    z3::expr_vector initial;
    z3::expr_vector transition; // vector of clauses (cnf)
    z3::expr_vector constraint; // vector of clauses (cnf)

   public:
    std::string name;

    mysat::primed::VarVec vars;
    mysat::primed::ExpVec property;
    mysat::primed::ExpVec n_property;

    IModel(z3::context& c, const std::set<std::string>& varnames)
        : ctx(c), initial(ctx), transition(ctx), constraint(ctx),
          vars(ctx, varnames), property(ctx, vars), n_property(ctx, vars)
    {
    }

    const z3::expr_vector& get_initial() const;
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_constraint() const;
    // return the number of pebbles in the final state
    void show(std::ostream& out) const;
  };
} // namespace pdr

#endif // !PDR_MODEL
