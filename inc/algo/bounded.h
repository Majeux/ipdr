#ifndef BOUNDED_H
#define BOUNDED_H

#include "dag.h"
#include <optional>
#include <z3++.h>

namespace bounded
{
  // clang-format off
  struct Literal
  {
    size_t index;
    enum polarity {positive, negative} sign;
    size_t time;

    operator bool() const { return sign == polarity::positive; }
  };
  // clang-format on

  struct ConstrainedExpr
  {
    z3::expr expression;
    z3::expr constraint;

    operator z3::expr_vector()
    {
      z3::expr_vector rv(expression.ctx());
      rv.push_back(expression);
      rv.push_back(constraint);
      return rv;
    }
  };

  class BoundedPebbling
  {
   public:
    // using Literals = std::vector<Literal>;
    using Literals = z3::expr_vector;

    std::vector<Literals> lits_at_time;

    BoundedPebbling(const dag::Graph& G);

    bool find_for(size_t pebbles);

   private:
    z3::context context;
    const dag::Graph& graph;
    z3::solver solver;
    size_t bt_points = 0;

    const std::set<std::string, std::less<>>& lit_names;
    const size_t n_lits;

    std::optional<size_t> cardinality;
    // the amount of steps that are added to the solver
    // the last transition is from `current_bound-1` to `current_bound`
    std::optional<size_t> current_bound;

    z3::expr lit(std::string_view name, size_t time_step);
    z3::expr constraint(const z3::expr_vector& lits);
    // empty state and cardinality clase (index 0)
    ConstrainedExpr initial();
    // final state for the last state and cardinality
    ConstrainedExpr final();
    // returns the transition for step i -> i+1
    // with the cardinality clause for step i+1
    ConstrainedExpr trans_step(size_t i);
    // transition relations for an amount of steps
    void push_transitions(size_t steps);

    z3::check_result check(size_t steps);

    void dump_strategy() const;

    void bt_push();
    void bt_pop();
  }; // class Bounded
} // namespace bounded

#endif // BOUNDED_H
