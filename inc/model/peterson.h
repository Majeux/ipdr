#ifndef PETERSON_H
#define PETERSON_H

#include <z3++.h>

namespace peterson
{
  class Model
  {
   public:
    Model(z3::config& settings, unsigned n_processes);

   private:
    z3::context ctx;

    unsigned N;           // no. processes
    z3::expr_vector pc;   // vector of ints[0-4]. program counter for process i
    z3::expr_vector l;    // vector of ints. level for process i
    z3::expr_vector last; // vector of ints. last process to enter level j

    z3::expr_vector initial;     // each array index to '-1;. pc to 0
    z3::expr_vector transition;

  }; // class Model
} // namespace peterson

#endif // PETERSON_H
