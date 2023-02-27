#include "z3-pebbling-model.h"
#include "logger.h"
#include "pdr-model.h"
#include "z3-ext.h"
#include <sstream>
#include <z3++.h>
#include <z3_spacer.h>

#define PAR_T true

namespace pdr::test
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::sort_vector;

  // Encoding transition system
  //
  Z3PebblingModel::Z3PebblingModel(
      my::cli::ArgumentList const& args, z3::context& c, dag::Graph const& G)
      : Z3Model(c, std::vector<string>(G.nodes.begin(), G.nodes.end())),
        dag(G),
        pebble_constraint{},
        initial(ctx),
        target(ctx),
        reach_rule(ctx),
        state_sorts(ctx),
        state(ctx),
        step(ctx)
  {
    // registering functions for states and transitions
    for (size_t i{ 0 }; i < G.nodes.size(); i++)
      state_sorts.push_back(ctx.bool_sort());

    state = z3::function("state", state_sorts, ctx.bool_sort());
    step  = z3::function(
         "step", z3ext::vec_add(state_sorts, state_sorts), ctx.bool_sort());

    prepare_initial();
    prepare_transitions();
    prepare_target();
  }

  Z3PebblingModel& Z3PebblingModel::constrained(
      std::optional<unsigned int> maximum_pebbles)
  {
    constrain(maximum_pebbles);
    return *this;
  }

  void Z3PebblingModel::add_initial(z3::fixedpoint& engine)
  {
    engine.add_rule(initial.expr, initial.name);
  }

  void Z3PebblingModel::add_transitions(z3::fixedpoint& engine)
  {
    engine.register_relation(state);
    if (!PAR_T)
    {
      engine.register_relation(step);
      engine.add_rule(reach_rule.expr, reach_rule.name);
    }

    for (Rule& r : rules)
      engine.add_rule(r.expr, r.name);
  }

  expr Z3PebblingModel::get_target() const { return target; }

  z3::check_result Z3PebblingModel::reach_target(z3::fixedpoint& engine)
  {
    return engine.query(target);
  }

  void Z3PebblingModel::constrain(std::optional<unsigned int> maximum_pebbles)
  {
    pebble_constraint = maximum_pebbles;
    rules.clear();
    prepare_transitions();
  }

  // Model definitions
  //
  std::string Z3PebblingModel::to_string() const
  {
    std::stringstream ss;
    ss << fmt::format("I: {}", initial.expr.to_string()) << std::endl
       << std::endl;
    ss << "Rules" << std::endl;
    for (Rule const& r : rules)
      ss << fmt::format("-- {}: {}", r.name.str(), r.expr.to_string())
         << std::endl;

    return ss.str();
  }

  expr Z3PebblingModel::make_constraint() const
  {
    if (pebble_constraint)
      return (z3::atmost(vars, *pebble_constraint));
    return z3_true;
  }

  expr Z3PebblingModel::constraint_assertion()
  {
    if (pebble_constraint)
      return forall_vars(z3::atmost(vars, *pebble_constraint));
    return z3_true;
  }

  expr_vector Z3PebblingModel::get_initial() const
  {
    return z3ext::transform(
        vars(), [this](expr const& e) { return e == z3_false; });
  }

  std::optional<unsigned> Z3PebblingModel::get_pebble_constraint() const
  {
    return pebble_constraint;
  }

  // horn rule: forall x1..x_n: head(x1..x_n) <= body(x1..xn)
  // if using functions: register the functions. then bind each rule with
  // universal quantifier over vars
  Z3PebblingModel::Rule Z3PebblingModel::pebbling_transition(
      expr const& parent, std::set<expr, z3ext::expr_less> const& children)
  {
    expr_vector args = z3ext::copy(vars()); // state(vars) -> vars in next
    for (expr const& e : vars())
    {
      if (e.id() == parent.id())
        args.push_back(!e); // flip parent
      else if (children.find(e) != children.end())
        args.push_back(z3_true); // keep children pebbled
      else
        args.push_back(e); // don't care about the rest
    }

    expr_vector child_vec = z3ext::mk_expr_vec(ctx, children);
    expr guard            = make_constraint();

    if (not child_vec.empty())
      guard = guard & z3::mk_and(child_vec);

    return mk_rule(
        step(args), guard, fmt::format("flip {}", parent.to_string()));
  }

  namespace
  {
    // gather all expressions in e, except those in basic
    void aux_var_rec(expr const& e, std::set<expr, z3ext::expr_less>& visited, expr_vector& aux,
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

  Z3PebblingModel::Rule Z3PebblingModel::parallel_pebbling_transitions()
  {
    expr guard = make_constraint();
    expr_vector T_conj(ctx);
    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      expr parent = vars(i);

      expr_vector stay_conj(ctx);
      for (string const& c : dag.get_children(parent.to_string()))
      {
        expr child = ctx.bool_const(c.c_str());
        stay_conj.push_back(child && vars.p(child));
      }

      expr flip = parent ^ vars.p(parent);
      expr stay = stay_conj.size() == 0 ? z3_true : z3::mk_and(stay_conj);

      T_conj.push_back(z3::implies(flip, stay));
    }
    expr T    = z3ext::tseytin::to_cnf(z3::mk_and(T_conj));
    expr horn = z3::implies(state(vars) && T && guard, state(vars.p()));
    expr_vector all_vars = get_all_vars(T, z3ext::vec_add(vars(), vars.p()));

    expr rule = z3::forall(all_vars, horn);
    // expr rule = z3::forall(z3ext::vec_add(vars(), vars.p()), horn);

    return { rule, ctx.str_symbol("T") };
  }

  void Z3PebblingModel::prepare_initial()
  {
    expr_vector I = get_initial();
    initial       = mk_rule(z3::implies(z3::mk_and(I), state(vars())), "I");
  }

  void Z3PebblingModel::prepare_transitions()
  {
    if (PAR_T)
    {
      rules.push_back(parallel_pebbling_transitions());
      return;
    }

    // register reachability from current to next state as a rule
    // state.p <= state && step(state, state.p)
    {
      z3::expr head   = state(vars.p());
      expr_vector all = z3ext::vec_add(vars(), vars.p());
      expr body       = state(vars()) && step(all);
      reach_rule      = mk_rule(z3::implies(body, head), "->");
    }

    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      expr parent = vars(i);

      std::set<expr, z3ext::expr_less> children;
      for (string const& c : dag.get_children(parent.to_string()))
        children.insert(ctx.bool_const(c.c_str()));

      rules.push_back(pebbling_transition(parent, children));
    }
  }

  void Z3PebblingModel::prepare_target()
  {
    expr_vector conj = z3ext::transform(vars(), [this](expr const& e)
        { return dag.is_output(e.to_string()) ? z3_true : z3_false; });

    target = state(conj);
  }
} // namespace pdr::test
