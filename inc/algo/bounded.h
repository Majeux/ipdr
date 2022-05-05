#ifndef BOUNDED_H
#define BOUNDED_H

#include "dag.h"
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

  class Bounded
  {
   public:
    // using Literals = std::vector<Literal>;
    using Literals = z3::expr_vector;

    std::vector<Literals> lits_at_time;

    Bounded(const dag::Graph& G);

    // transition relations for an amount of steps
    z3::expr_vector transition(size_t steps);

    // formula for an amount of steps
    z3::expr iteration(size_t steps);

   private:
    z3::context context;
    const dag::Graph& graph;
    z3::solver solver;
    size_t bt_points = 0;

    const std::set<std::string>& lit_names;
    const size_t n_lits;

    z3::expr lit(std::string_view name, size_t time);
    void push_time_frame();
    // returns the transition for step i -> i+1
    // with the cardinality clause for step i+1
    z3::expr_vector next_trans(size_t i);
    // returns the transition for step i -> k
    // with the cardinality clause for step k
    z3::expr_vector final_trans(size_t i);

    void bt_push();
    void bt_pop();
  }; // class Bounded
} // namespace bounded

#endif // BOUNDED_H
