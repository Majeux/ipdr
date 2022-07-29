#ifndef PDR_MODEL
#define PDR_MODEL

#include <z3++.h>

#include "exp-cache.h"

namespace pdr
{
  class IModel
  {
   public:
    std::string name;
    z3::context ctx;
    ExpressionCache property;
    ExpressionCache n_property;

    IModel(z3::config& settings)
        : ctx(settings), property(ctx), n_property(ctx), initial(ctx),
          transition(ctx)
    {
    }

    // parametrized addition to property
    virtual z3::expr_vector constraint(std::optional<unsigned> x);
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
