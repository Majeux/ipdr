#include "bounded.h"
#include "dag.h"

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
    for (string_view n : lit_names)
      solver.add(!lit(n, 0));

    solver.add(F);
    bt_push(); // backtracking point to no transitions or cardinality
  }

  expr Bounded::lit(std::string_view name, size_t time_step)
  {
    std::string full_name = fmt::format("{}[{}]", name, time);
    return context.bool_const(full_name.c_str());
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
    {
      rv.push_back(T_i, T_i + 1);
      rv.push_back(card_i + 1);
      bt_push();
    }
    return rv;
  }

  expr_vector Bounded::final_trans(size_t i)
  {
    expr_vector rv(context);
    {
      rv.push_back(T_i, T_f);
      rv.push_back(card__f);
      bt_push();
    }
    return rv;
  }

  expr_vector Bounded::transition(size_t steps)
  {
    for (size_t i = 0; i < steps; i++)
    {
      expr_vector new_t = i < steps - 1 ? next_trans(i) : final_trans(i);
      assert(new_t.size() == 2);
      solver.add(new_t[0]); // transition
      solver.add(new_t[1]); // cardinality
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
