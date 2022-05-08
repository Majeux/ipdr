#include "bounded.h"
#include "dag.h"
#include <z3++.h>

namespace bounded
{
  using std::string;
  using std::string_view;
  using z3::expr;
  using z3::expr_vector;

  Bounded::Bounded(const dag::Graph& G)
      : context(), graph(G), solver(context), lit_names(G.nodes),
        n_lits(lit_names.size())
  {
    ConstrainedExpr I = initial();
    solver.add(I.expression);
    solver.add(I.constraint);
    current_bound = 0;
    bt_push(); // backtracking point to no transitions or cardinality
  }

  expr Bounded::lit(std::string_view name, size_t time_step)
  {
    std::string full_name = fmt::format("{}.{}", name, time_step);
    return context.bool_const(full_name.c_str());
  }

  expr Bounded::constraint(const expr_vector& lits)
  {
    return z3::atmost(lits, cardinality.value());
  }

  ConstrainedExpr Bounded::initial()
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

  ConstrainedExpr Bounded::final(size_t i)
  {
    expr_vector cube(context), lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, i);
      lits.push_back(l);
      if (graph.is_output(n))
        cube.push_back(l);
      else
        cube.push_back(!l);
    }

    return { z3::mk_and(cube), constraint(lits) };
  }

  void Bounded::push_time_frame()
  {
    Literals v(context);
    // v.reserve(n_lits);
    const size_t step = lits_at_time.size();

    for (std::string_view n : graph.nodes)
      v.push_back(lit(n, step));

    lits_at_time.push_back(v);
  }

  ConstrainedExpr Bounded::trans_step(size_t i)
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

  z3::check_result Bounded::check(size_t steps)
  {
    push_transitions(steps);
    z3::check_result result = solver.check(final(steps));
    assert(result != z3::check_result::unknown);
    return result;
  }

  void Bounded::push_transitions(size_t steps)
  {
    for (size_t i = current_bound; i < steps; i++)
    {
      expr_vector new_t = trans_step(i);
      assert(new_t.size() == 2);
      solver.add(new_t);
    }

    current_bound = steps - 1;
  }

  expr Bounded::iteration(size_t steps) {}

  void Bounded::bt_push()
  {
    solver.push();
    bt_points++;
  }

  void Bounded::bt_pop()
  {
    solver.pop();
    bt_points--;
  }

} // namespace bounded
