#include "bounded.h"

namespace bounded
{
  using z3::expr;
  using z3::expr_vector;

  Bounded::Bounded() : context(), solver(context)
  {
    solver.add(I);
    solver.add(F);
    bt_push(); // backtracking point to no transitions or cardinality
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
      expr_vector new_t = i < steps-1 ? next_trans(i) : final_trans(i);
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
