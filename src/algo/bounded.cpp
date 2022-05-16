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
    solver.add(initial());
    bt_push(); // backtracking point to no transitions or cardinality
  }

  expr Bounded::lit(std::string_view name, size_t time_step)
  {
    std::string full_name = fmt::format("{}.{}", name, time_step);
    return context.bool_const(full_name.c_str());
  }

  expr Bounded::initial()
  {
    expr_vector cube(context);
    expr_vector lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, 0);
      lits.push_back(l);
      cube.push_back(!l);
    }
    cube.push_back(z3::atmost(lits, cardinality.value()));

    return z3::mk_and(cube);
  }

  expr_vector Bounded::final(size_t i)
  {
    expr_vector cube(context);
    expr_vector lits(context);
    for (string_view n : lit_names)
    {
      expr l = lit(n, i);
      lits.push_back(l);
      if (graph.is_output(n))
        cube.push_back(l);
      else
        cube.push_back(!l);
    }
    cube.push_back(z3::atmost(lits, cardinality.value()));

    return cube;
  }

  void Bounded::push_time_frame()
  {
    Literals v(context);
    // v.reserve(n_lits);
    const size_t step = lits_at_time.size();

    for (std::string_view n : graph.nodes)
    {
      std::string name = fmt::format("{}[{}]", n, step);
      z3::expr l       = context.bool_const(name.c_str());
      v.push_back(l);
    }

    lits_at_time.push_back(v);
  }

  expr_vector Bounded::next_trans(size_t i)
  {
    expr_vector rv(context);
    expr_vector current(context);
    expr_vector next(context);

    expr_vector T(context);
    for (std::string_view node : lit_names) // every node has a transition
    {
      expr source   = lit(node, i);
      expr source_p = lit(node, i+1);
      current.push_back(source);
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

    rv.push_back(z3::mk_and(T));
    rv.push_back(z3::atmost(next, cardinality.value()));
    return rv;
  }

  void Bounded::check(size_t steps)
  {
    // fill transitions until steps-1
    solver.check(final(steps));
  }

  void Bounded::transition(size_t steps)
  {
    for (size_t i = 0; i < steps; i++)
    {
      expr_vector new_t = next_trans(i);
      assert(new_t.size() == 2);
      solver.add(new_t);
    }
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
