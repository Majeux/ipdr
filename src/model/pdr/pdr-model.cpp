#include "pdr-model.h"
#include "cli-parse.h"
#include <TextTable.h>
#include <numeric>
#include <optional>
#include <z3++.h>
#include <z3_api.h>

namespace pdr
{
  using std::string;
  using z3::expr;
  using z3::expr_vector;

  IModel::IModel(z3::context& c, const std::vector<std::string>& varnames)
      : ctx(c),
        vars(ctx, varnames),
        property(ctx, vars),
        n_property(ctx, vars),
        state(ctx),
        initial(ctx),
        transition(ctx),
        constraint(ctx),
        state_sorts(ctx)
  {
  }

  //
  const z3::expr_vector& IModel::get_initial() const { return initial; }
  const z3::expr_vector& IModel::get_transition() const { return transition; }
  const z3::expr_vector& IModel::get_constraint() const { return constraint; }

  // fixedpoint interface
  //
  namespace
  {
    expr cube_to_assignment(expr_vector const& cube)
    {
      expr z3_true  = cube.ctx().bool_val(true);
      expr z3_false = cube.ctx().bool_val(false);

      expr_vector rv(cube.ctx());
      for (expr const& e : cube)
      {
        if (!z3ext::is_lit(e))
          throw std::runtime_error("Need a cube to turn into assignment.");
        rv.push_back(e.is_not() ? e.arg(0) == z3_false : e == z3_true);
      }
      return z3::mk_and(rv);
    }

    expr cube_to_state(expr_vector const& cube, z3::func_decl const& state)
    {
      expr z3_true  = cube.ctx().bool_val(true);
      expr z3_false = cube.ctx().bool_val(false);

      expr_vector args(cube.ctx());
      for (expr const& e : cube)
      {
        if (!z3ext::is_lit(e))
          throw std::runtime_error("Need a cube to turn into assignment.");
        args.push_back(e.is_not() ? z3_false : z3_true);
      }
      return state(args);
    }

    // gather all expressions in e, except those in basic
    void aux_var_rec(expr const& e,
        std::set<expr, z3ext::expr_less>& visited,
        expr_vector& aux,
        std::set<expr, z3ext::expr_less> const& basic)
    {
      // already visited or is one the basic variables
      if (visited.insert(e).second == false || basic.find(e) != basic.end())
        return;

      if (e.is_const())
      {
        aux.push_back(e);
      }
      else if (e.is_app())
      {
        for (size_t i{ 0 }; i < e.num_args(); i++)
          aux_var_rec(e.arg(i), visited, aux, basic);
      }
      else if (e.is_quantifier())
      {
        aux_var_rec(e.body(), visited, aux, basic);
      }
    }

    // gather all auxiliary variables in e (introduced by tseitin.
    // return vars() + vars.p() + aux_vars
    expr_vector get_all_vars(expr e, expr_vector const& basic)
    {
      expr_vector aux(basic);
      std::set<expr, z3ext::expr_less> visited, basic_set;
      for (expr const& e : basic)
        basic_set.insert(e);

      aux_var_rec(e, visited, aux, basic_set);

      return z3ext::vec_add(basic, aux);
    }
  } // namespace

  void IModel::load_initial(z3::fixedpoint& engine)
  {
    state_sorts.resize(0);
    for (size_t i{ 0 }; i < state_size(); i++)
      state_sorts.push_back(ctx.bool_sort());

    state = z3::function("state", state_sorts, ctx.bool_sort());
    engine.register_relation(state);
    fp_I = mk_rule(
        z3::implies(cube_to_assignment(get_initial()), state(vars())), "I");
    engine.add_rule(fp_I->expr, fp_I->name);
  }

  void IModel::load_transition(z3::fixedpoint& engine)
  {
    expr guard = get_constraint_current();
    expr trans = z3::mk_and(get_transition());
    expr horn  = z3::implies(state(vars()) && trans && guard, state(vars.p()));
    // collect potential auxiliary vars not in the state (such as from tseytin)
    expr_vector all_vars = get_all_vars(horn, z3ext::vec_add(vars(), vars.p()));

    fp_T.push_back({ z3::forall(all_vars, horn), ctx.str_symbol("T") });
    engine.add_rule(fp_T.back().expr, fp_T.back().name);
  }

  z3::expr IModel::create_fp_target()
  {
    return z3::exists(vars(), state(vars) && z3::mk_and(n_property()));
  }

  void IModel::show(std::ostream& out) const
  {
    using std::endl;

    unsigned t_size = std::accumulate(transition.begin(), transition.end(), 0,
        [](unsigned acc, const z3::expr& e) { return acc + e.num_args(); });

    out << fmt::format("Transition Relation (size = {}):", t_size) << endl
        << transition << endl;

    out << endl << "property: " << endl;
    for (std::string_view s : property.names())
      out << s << endl;

    out << endl << "neg_property: " << endl;
    for (std::string_view s : n_property.names())
      out << s << endl;
  }

  IModel::Rule IModel::mk_rule(expr const& e, string const& n)
  {
    return { forall_vars(e), ctx.str_symbol(n.c_str()) };
  }

  IModel::Rule IModel::mk_rule(
      expr const& head, expr const& body, string const& n)
  {
    return { forall_vars(z3::implies(body, head)), ctx.str_symbol(n.c_str()) };
  }

  IModel::Rule IModel::mk_rule_aux(
      expr const& head, expr const& body, string const& n)
  {
    expr horn            = z3::implies(body, head);
    expr_vector all_vars = get_all_vars(horn, z3ext::vec_add(vars(), vars.p()));
    return { z3::forall(all_vars, horn), ctx.str_symbol(n.c_str()) };
  }

  expr IModel::forall_vars(expr const& e) const
  {
    return z3::forall(z3ext::vec_add(vars(), vars.p()), e);
  }

} // namespace pdr
