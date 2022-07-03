#ifndef PETERSON_H
#define PETERSON_H

#include "exp-cache.h"
#include "expr.h"
#include <z3++.h>

namespace peterson
{

  class Model
  {
   public:
    using IStays   = mysat::primed::IStays;
    using Lit      = mysat::primed::Lit;
    using BitVec   = mysat::primed::BitVec;
    using Array    = mysat::primed::Array;
    using numrep_t = BitVec::numrep_t;

    Model(size_t Nbits, z3::config& settings, numrep_t n_processes);

   private:
    z3::context ctx;
    size_t nbits;
    // no. processes
    numrep_t N;
    // vector of ints[0-4]. program counter for process i
    std::vector<BitVec> pc;
    // vector of ints. level for process i
    std::vector<BitVec> l;
    // flag that denotes if process i has released the resource
    // alternatively viewed as a sign bit for l
    std::vector<Lit> free;
    // int array. last process to enter level j
    std::vector<BitVec> last;
    // Array last;

    z3::expr_vector initial;    // each array index to '-1;. pc to 0
    z3::expr transition; // converted into cnf via tseytin

    void bitvector_test(size_t max_value);
    z3::expr T_start(numrep_t i);
    z3::expr T_boundcheck(numrep_t i);
    z3::expr T_setlast(numrep_t i);
    z3::expr T_await(numrep_t i);
    z3::expr T_release(numrep_t i);
  }; // class Model
} // namespace peterson

#endif // PETERSON_H
