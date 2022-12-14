#include "z3pdr.h"
#include "dag.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "string-ext.h"
#include "z3-ext.h"

#include <algorithm>
#include <fmt/core.h>
#include <iterator>
#include <numeric>
#include <z3++.h>
#include <z3_spacer.h>

namespace pdr::test
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::sort_vector;

  z3PDR::Rule z3PDR::mk_rule(expr const& e, string const& n)
  {
    return { forall_vars(e), ctx().str_symbol(n.c_str()) };
  }

  z3PDR::Rule z3PDR::mk_rule(
      expr const& head, expr const& body, string const& n)
  {
    return { forall_vars(z3::implies(body, head)),
      ctx().str_symbol(n.c_str()) };
  }

  expr z3PDR::forall_vars(expr const& e) const
  {
    return z3::forall(z3ext::vec_add(vars(), vars.p()), e);
  }

  // horn clause: head <= body
  //              state && state.p <=
  z3PDR::Rule z3PDR::pebbling_transition(
      expr const& parent, std::set<expr, z3ext::expr_less> const& children)
  {
    assert(parent);

    expr_vector args = z3ext::copy(vars());
    for (expr const& e : vars())
    {
      if (e.id() == parent.id())
        args.push_back(!e); // flip parent
      else if (children.find(e) != children.end())
        args.push_back(z3_true); // keep children pebbled
      else
        args.push_back(e); // don't care about the rest
    }

    expr_vector child_vec = z3ext::make_expr_vec(ctx, children);
    expr guard            = child_vec.empty() ? z3_true : z3::mk_and(child_vec);

    return mk_rule(
        step(args), guard, fmt::format("flip {}", parent.to_string()));
  }

  // done in setup, defines relation between state and next state
  // if step(state, state.p) |-> true && state, then state.p
  z3PDR::z3PDR(Context& c, Logger& l, dag::Graph const& G)
      : ctx(c), log(l), engine(ctx),
        vars(ctx, vector<string>(G.nodes.begin(), G.nodes.end())), target(ctx),
        state_sorts(ctx), state(ctx), step(ctx), reach_rule(ctx), initial(ctx)
  {
    {
      z3::params p(ctx);
      p.set("engine", "spacer");                 // z3 pdr implementation
      p.set("spacer.push_pob", true);            // pushing blocked facts
      p.set("spacer.p3.share_invariants", true); // invariant lemmas
      p.set("spacer.p3.share_lemmas", true);     // frame lemmas (clauses?)
      engine.set(p);
      std::cout << "Settings" << std::endl
                << engine.get_param_descrs() << std::endl
                << std::endl;
      std::cout << "z3::fixedpoint options" << std::endl
                << str::ext::indent(engine.help()) << std::endl
                << std::endl;
    }

    { // registering functions for states and transitions
      for (size_t i{ 0 }; i < G.nodes.size(); i++)
        state_sorts.push_back(ctx().bool_sort());

      state = z3::function("state", state_sorts, ctx().bool_sort());
      step  = z3::function(
           "step", z3ext::vec_add(state_sorts, state_sorts), ctx().bool_sort());

      engine.register_relation(state);
      engine.register_relation(step);
    }

    // register reachability from current to next state as a rule
    // state.p <= state && step(state, state.p)
    {
      z3::expr head   = state(vars.p());
      expr_vector all = z3ext::vec_add(vars(), vars.p());
      expr body       = state(vars()) && step(all);
      reach_rule      = mk_rule(z3::implies(body, head), "reach");

      engine.add_rule(reach_rule.expr, reach_rule.name);
    }

    // initial state
    {
      expr_vector I =
          z3ext::transform(vars(), [](expr const& e) { return !e; });
      initial = mk_rule(z3::implies(z3::mk_and(I), state(vars())), "I");

      engine.add_rule(initial.expr, initial.name);
    }

    // transitions
    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      expr parent = vars(i);

      std::set<expr, z3ext::expr_less> children;
      for (string const& c : G.get_children(parent.to_string()))
        children.insert(ctx().bool_const(c.c_str()));

      rules.push_back(pebbling_transition(parent, children));
    }

    // target state
    expr_vector t_conj(ctx);
    for (expr const& e : vars())
    {
      if (G.is_output(e.to_string()))
        t_conj.push_back(e);
      else
        t_conj.push_back(!e);
    }
    target = state(vars()) && z3::mk_and(t_conj);
    // target_decl = target.decl();
    // engine.register_relation(target_decl);
  }

  // horn_rule ::= (forall (bound-vars) horn_rule)
  //            |  (=> and(atoms) horn_rule)
  //            |  atom
  // rule(head, body) = mk_and(body) => head
  // fact(head) = rule(head, true)
  //
  // if using functions: register the functions. then bind each rule with
  // Existential quantifier over vars
  void z3PDR::run()
  {
    using std::cout;
    using std::endl;

    cout << fmt::format("I: {}", initial.expr.to_string()) << endl << endl;

    cout << "Rules" << endl;
    for (Rule& r : rules)
    {
      cout << fmt::format("-- {}: {}", r.name.str(), r.expr.to_string())
           << endl;
      engine.add_rule(r.expr, r.name);
    }

    cout << endl << "Target" << endl;
    cout << target.to_string() << endl << endl;

    cout << "Fixedpoint engine" << endl << engine.to_string() << endl;

    z3::check_result r = engine.query(target);

    switch (r)
    {
      case z3::check_result::sat: cout << "sat fixed point"; break;
      case z3::check_result::unsat: cout << "unsat fixed point"; break;
      case z3::check_result::unknown: cout << "unknown fixed point"; break;
    }

    cout << "trace:" << endl;
    cout << Z3_fixedpoint_get_rule_names_along_trace(ctx(), engine) << endl;
  }
} // namespace pdr::test
