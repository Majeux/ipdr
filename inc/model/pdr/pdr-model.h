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
    z3::context& ctx;
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

    IModel(z3::context& c, const std::vector<std::string>& varnames);
    virtual ~IModel() {}

    const z3::expr_vector& get_initial() const;
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_constraint() const;
    // brief string to describe the meaning of the present constraint
    virtual const std::string constraint_str() const = 0;
    // number representative of the constraint. a larger number is a looser
    // constraint
    virtual unsigned constraint_num() const = 0;
    // return the number of pebbles in the final state
    void show(std::ostream& out) const;

   protected:
    z3::expr_vector initial;
    z3::expr_vector transition; // vector of clauses (cnf)
    z3::expr_vector constraint; // vector of clauses (cnf)
  };

  // an interface for a model to be verified using z3's fixedpoint engine
  class Z3Model
  {
   public:
    z3::context& ctx;
    std::string name;

    mysat::primed::VarVec vars;

    Z3Model(z3::context& c, const std::vector<std::string>& varnames);
    virtual ~Z3Model();

    virtual void add_initial(z3::fixedpoint& engine)              = 0;
    virtual void add_transitions(z3::fixedpoint& engine)          = 0;
    virtual z3::expr get_target() const                           = 0;
    virtual z3::check_result reach_target(z3::fixedpoint& engine) = 0;

    void show(std::ostream& out) const;
    virtual std::string to_string() const = 0;

   protected:
    struct Rule
    {
      z3::expr expr;
      z3::symbol name;

      Rule(z3::context& ctx) : expr(ctx), name(ctx.str_symbol("empty str")) {}
      Rule(z3::expr const& e, z3::symbol const& n) : expr(e), name(n) {}
    };

    // create a rule for the fixedpoint engine
    Rule mk_rule(z3::expr const& e, std::string const& n);
    Rule mk_rule(
        z3::expr const& head, z3::expr const& body, std::string const& n);

    // quantify over all variables in vars
    z3::expr forall_vars(z3::expr const& e) const;
  };
} // namespace pdr

#endif // !PDR_MODEL
