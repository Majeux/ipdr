#ifndef PETERSON_H
#define PETERSON_H

#include "exp-cache.h"
#include <z3++.h>

namespace peterson
{
  class Model
  {
   public:
    Model(z3::config& settings, unsigned n_processes);

   private:
    z3::context ctx;
    void stays(const PrimedExpressions& E, z3::expr_vector& add_to);
    void stays_except(
        const PrimedExpressions& E, z3::expr_vector& add_to, size_t exception);
    z3::expr T_start(unsigned i);
    z3::expr T_boundcheckfail(unsigned i);
    z3::expr T_boundchecksucc(unsigned i);
    z3::expr T_setlast(unsigned i);

    unsigned N;            // no. processes
    PrimedExpressions pc;  // vector of ints[0-4]. program counter for process i
    PrimedExpressions l;   // vector of ints. level for process i
    PrimedExpression last; // int array. last process to enter level j

    z3::expr_vector initial;    // each array index to '-1;. pc to 0
    z3::expr_vector transition; // or of ands, all possible transitions

    z3::expr x;
    z3::expr array_range;
  }; // class Model
} // namespace peterson

#endif // PETERSON_H
