#ifndef PETERSON_H
#define PETERSON_H

#include <z3++.h>

#include "exp-cache.h"
#include "expr.h"
#include "pdr-model.h"

namespace peterson
{
  struct State;

  class Model : public pdr::IModel
  {
   public:
    using IStays   = mysat::primed::IStays;
    using Lit      = mysat::primed::Lit;
    using BitVec   = mysat::primed::BitVec;
    using numrep_t = BitVec::numrep_t;

    friend State;

    Model(z3::context& c, numrep_t n_processes);

   private:
    // no. processes
    numrep_t N;
    // vector of ints[0-4]. program counter for process i
    std::vector<BitVec> pc;
    const static numrep_t pc_num = 5;
    // vector of ints. level for process i
    std::vector<BitVec> level;
    // flag that denotes if process i has released the resource
    // alternatively viewed as a sign bit for l
    std::vector<Lit> free;
    // int array. last process to enter level j
    std::vector<BitVec> last;

    // z3::expr_vector initial;    // each array index to '-1;. pc to 0
    // z3::expr_vector transition; // converted into cnf via tseytin

    std::set<std::string> create_vars();

    State extract_state(const z3::expr_vector& witness,
        mysat::primed::lit_type t = mysat::primed::lit_type::base);
    State extract_state_p(const z3::expr_vector& witness);

    std::set<State> successors(const z3::expr_vector& v);
    std::set<State> successors(const State& s);

    void test_room();
    void test_wait(numrep_t i);

    z3::expr T_start(numrep_t i);
    z3::expr T_boundcheck(numrep_t i);
    z3::expr T_setlast(numrep_t i);
    z3::expr T_await(numrep_t i);
    z3::expr T_release(numrep_t i);

    void bv_comp_test(size_t max_value);
    void bv_val_test(size_t max_value);
  }; // class Model

  struct State
  {
    std::vector<Model::numrep_t> pc;
    std::vector<Model::numrep_t> level;
    std::vector<bool> free;
    std::vector<Model::numrep_t> last;

    // TODO use model vector sizes
    State() : pc(0), level(0), free(0), last(0) {}
    State(Model::numrep_t N) : pc(N), level(N), free(N), last(N - 1) {}
    State(std::vector<Model::numrep_t>&& p, std::vector<Model::numrep_t>&& l,
        std::vector<bool>&& f, std::vector<Model::numrep_t>&& lst)
        : pc(p), level(l), free(f), last(lst)
    {
    }

    z3::expr_vector cube(Model& m) const;
    std::string to_string(bool inl = false) const;
    std::string inline_string() const;

    bool operator<(const State&) const;
    bool operator==(const State&) const;
    bool operator!=(const State&) const;
  };

} // namespace peterson

#endif // PETERSON_H
