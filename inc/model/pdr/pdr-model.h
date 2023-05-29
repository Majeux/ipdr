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

    // cnf representation of transition system
    //
    const z3::expr_vector& get_initial() const;
    const z3::expr_vector& get_transition() const;
    const z3::expr_vector& get_constraint() const;
    virtual const z3::expr get_constraint_current() const = 0;

    // load horn-clause representation into a z3::fixedpoint engine.
    // uses cnf representations by default, can be overriden to specialize
    //
    virtual void load_initial(z3::fixedpoint& engine);
    virtual void load_transition(z3::fixedpoint& engine);
    virtual z3::expr create_fp_target();

    // the number of literals that encode a state of the system
    virtual unsigned state_size() const              = 0;
    // brief string to describe the meaning of the present constraint
    virtual const std::string constraint_str() const = 0;
    // number representative of the constraint. a larger number is a looser
    // constraint
    virtual unsigned constraint_num() const          = 0;
    // return the number of pebbles in the final state
    void show(std::ostream& out) const;

    z3::func_decl state; // B^N |-> B
   protected:
    z3::expr_vector initial;
    z3::expr_vector transition; // vector of clauses (cnf)
    z3::expr_vector constraint; // vector of clauses (cnf)

    // fixedpoint engine interface
    //
    z3::sort_vector state_sorts;

    struct Rule
    {
      z3::expr expr;
      z3::symbol name;

      Rule(z3::context& ctx) : expr(ctx), name(ctx.str_symbol("empty str")) {}
      Rule(z3::expr const& e, z3::symbol const& n) : expr(e), name(n) {}
    };
    std::optional<Rule> fp_I;
    std::vector<Rule> fp_T;

    // create a rule for the fixedpoint engine
    Rule mk_rule(z3::expr const& e, std::string const& n);
    Rule mk_rule(
        z3::expr const& head, z3::expr const& body, std::string const& n);
    Rule mk_rule_aux(
        z3::expr const& head, z3::expr const& body, std::string const& n);
    // quantify over all variables in vars
    z3::expr forall_vars(z3::expr const& e) const;
  };
} // namespace pdr

#endif // !PDR_MODEL
