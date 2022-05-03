#ifndef BOUNDED_H
#define BOUNDED_H

#include <z3++.h>

namespace bounded 
{
  class Bounded
  {
   public:
    Bounded();

    // transition relations for an amount of steps
    z3::expr_vector transition(size_t steps);

    // formula for an amount of steps
    z3::expr iteration(size_t steps);
    
   private:
    z3::context context;
    z3::solver solver;
    size_t bt_points = 0;

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
