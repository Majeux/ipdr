#ifndef PDR_MODEL
#define PDR_MODEL

#include <optional>
#include <string>
#include <vector>
#include <z3++.h>

#include "expr.h"

namespace pdr
{
  class IModel
  {
   public:
    z3::context ctx;
    std::string name;

    mysat::primed::VarVec vars;
    mysat::primed::ExpVec property;
    mysat::primed::ExpVec n_property;

    enum class Diff_t
    {
      none,
      constrained,
      relaxed
    };
    Diff_t diff{ Diff_t::none };

    IModel(const std::vector<std::string>& varnames);

    const z3::expr_vector& get_initial() const;
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_constraint() const;
    virtual const std::string constraint_str() const = 0;
    // return the number of pebbles in the final state
    void show(std::ostream& out) const;

   protected:
    z3::expr_vector initial;
    z3::expr_vector transition; // vector of clauses (cnf)
    z3::expr_vector constraint; // vector of clauses (cnf)
  };
} // namespace pdr

#endif // !PDR_MODEL
