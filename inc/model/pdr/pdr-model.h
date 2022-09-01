#ifndef PDR_MODEL
#define PDR_MODEL

#include <optional>
#include <z3++.h>

#include "expr.h"

namespace pdr
{
  class IModel
  {
   public:
    std::string name;
    z3::context ctx;
    mysat::primed::VarVec vars;
    mysat::primed::ExpVec property;
    mysat::primed::ExpVec n_property;

    IModel(z3::config& settings, const std::set<std::string>& varnames)
        : ctx(settings), vars(ctx, varnames), property(ctx, vars),
          n_property(ctx, vars), initial(ctx), transition(ctx)
    {
    }

    // parametrized addition to property
    virtual std::pair<z3::expr_vector, z3::expr_vector> constraint(
        std::optional<unsigned> x);
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_initial() const;
    // return the number of pebbles in the final state
    void show(std::ostream& out) const;

   protected:
    z3::expr_vector initial;
    z3::expr_vector transition; // vector of clauses (cnf)

    z3::config& set_config(z3::config& settings);
  };
} // namespace pdr

#endif // !PDR_MODEL
