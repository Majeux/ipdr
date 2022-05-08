#include "bounded.h"
#include "dag.h"
#include <z3++.h>

namespace bounded
{
  using std::string;
  using std::string_view;
  using z3::expr;
  using z3::expr_vector;

  BoundedPebbling::BoundedPebbling(const dag::Graph& G)
      : context(), graph(G), solver(context), lit_names(G.nodes),
        n_lits(lit_names.size())
  {
    context.set("timeout", 120000);
    context.set("model", true);
    solver.set("sat.cardinality.solver", true);
    solver.set("cardinality.solver", true);
    // solver.set("sat.random_seed", ctx.seed);
  }

  expr BoundedPebbling::lit(std::string_view name, size_t time_step)
  {
    std::string full_name = fmt::format("{}.{}", name, time_step);
    return context.bool_const(full_name.c_str());
  }

  expr BoundedPebbling::constraint(const expr_vector& lits)
  {
    return z3::atmost(lits, cardinality.value());
  }

  ConstrainedExpr BoundedPebbling::initial()
  {
    expr_vector cube(context), lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, 0);
      lits.push_back(l);
      cube.push_back(!l);
    }

    return { z3::mk_and(cube), constraint(lits) };
  }

  ConstrainedExpr BoundedPebbling::final()
  {
    expr_vector cube(context), lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, current_bound.value());
      lits.push_back(l);
      if (graph.is_output(n))
        cube.push_back(l);
      else
        cube.push_back(!l);
    }

    return { z3::mk_and(cube), constraint(lits) };
  }

  ConstrainedExpr BoundedPebbling::trans_step(size_t i)
  {
    expr_vector next(context), T(context);
    for (std::string_view node : lit_names) // every node has a transition
    {
      expr source   = lit(node, i);
      expr source_p = lit(node, i + 1);
      next.push_back(source_p);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (std::string_view child : graph.get_children(node))
      {
        // clang-format off
        T.push_back( source || !source_p || lit(child, i));
        T.push_back(!source ||  source_p || lit(child, i));
        T.push_back( source || !source_p || lit(child, i+1));
        T.push_back(!source ||  source_p || lit(child, i+1));
        // clang-format on
      }
    }

    return { z3::mk_and(T), constraint(next) };
  }

  z3::check_result BoundedPebbling::check(size_t steps)
  {
    push_transitions(steps);
    z3::check_result result = solver.check(final());
    assert(result != z3::check_result::unknown);
    return result;
  }

  void BoundedPebbling::push_transitions(size_t steps)
  {
    assert(steps > 0);
    if (!current_bound)
    {
      ConstrainedExpr I = initial();
      solver.add(I.expression);
      solver.add(I.constraint);
      current_bound = 0;
    }

    for (size_t& i = current_bound.value(); i < steps; i++)
      solver.add(trans_step(i));
  }

  bool BoundedPebbling::find_for(size_t pebbles)
  {
    for (; true; pebbles++)
    {
      cardinality = pebbles;
      for (size_t i = 1;; i++)
      {
        z3::check_result r = check(i);
        if (r == z3::check_result::sat)
        {
          dump_strategy();
          return true;
        }
      }
    }
    return false;
  }

  void BoundedPebbling::dump_strategy() const
  {
    z3::model witness = solver.get_model();
    std::cout << witness << std::endl;
    std::regex; state("*.[0-9]+");
  }

  void BoundedPebbling::bt_push()
  {
    solver.push();
    bt_points++;
  }

  void BoundedPebbling::bt_pop()
  {
    solver.pop();
    bt_points--;
  }

} // namespace bounded
