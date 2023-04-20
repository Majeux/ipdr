#ifndef PETERSON_H
#define PETERSON_H

#include <z3++.h>

#include "expr.h"
#include "pdr-model.h"

namespace pdr::peterson
{
  struct PetersonState;
  struct Vars;

  // encode the peterson algorithm for mutual exclusion as a cnf formula
  // for p processes, to a maximum of N
  //
  // I:
  class PetersonModel : public pdr::IModel
  {
   public:
    using IStays   = mysat::primed::IStays;
    using Lit      = mysat::primed::Lit;
    using BitVec   = mysat::primed::BitVec;
    using numrep_t = BitVec::numrep_t;

    friend PetersonState;

    PetersonModel(z3::context& c, numrep_t n_procs, numrep_t max_procs);

    const std::string constraint_str() const override;
    unsigned constraint_num() const override;
    unsigned n_processes() const;
    unsigned max_processes() const;

    // Configure IModel
    void constrain(numrep_t processes);

    // Convert a cube (typically a witness from a SAT call) to a state
    PetersonState extract_state(const z3::expr_vector& witness,
        mysat::primed::lit_type t = mysat::primed::lit_type::base) const;
    PetersonState extract_state_p(const z3::expr_vector& witness) const;


   private:
    // max no. processes. the size of the waiting queue
    const numrep_t N;
    // no. processes that can fire
    numrep_t p;
    BitVec nproc;
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

    size_t n_lits() const;

    // z3::expr_vector initial;    // each array index to '-1;. pc to 0
    // z3::expr_vector transition; // converted into cnf via tseytin

    // fill the pc, level, free and last variables
    Vars create_vars();

    std::set<PetersonState> successors(const z3::expr_vector& v);
    std::set<PetersonState> successors(const PetersonState& s);

    void test_room();
    void test_wait(numrep_t i);
    void test_bug();
    void test_property();
    void test_p_pred();

    void set_trans(numrep_t max_p);
    z3::expr T_start(numrep_t i);
    z3::expr T_boundcheck(numrep_t i);
    z3::expr T_setlast(numrep_t i);
    z3::expr T_await(numrep_t i);
    z3::expr T_release(numrep_t i);

    void bv_comp_test(size_t max_value);
    void bv_val_test(size_t max_value);
  }; // class Model

  struct PetersonState
  {
    std::vector<PetersonModel::numrep_t> pc;
    std::vector<PetersonModel::numrep_t> level;
    std::vector<bool> free;
    std::vector<PetersonModel::numrep_t> last;

    // TODO use model vector sizes
    PetersonState() : pc(0), level(0), free(0), last(0) {}
    PetersonState(PetersonModel::numrep_t N)
        : pc(N), level(N), free(N), last(N - 1)
    {
    }
    PetersonState(std::vector<PetersonModel::numrep_t>&& p,
        std::vector<PetersonModel::numrep_t>&& l, std::vector<bool>&& f,
        std::vector<PetersonModel::numrep_t>&& lst)
        : pc(p), level(l), free(f), last(lst)
    {
    }

    z3::expr_vector cube(PetersonModel& m) const;
    std::string to_string(bool inl = false) const;
    std::string inline_string() const;

    bool operator<(const PetersonState&) const;
    bool operator==(const PetersonState&) const;
    bool operator!=(const PetersonState&) const;
  };

  struct Vars
  {
    std::vector<std::string> curr;
    std::vector<std::string> next;
  };

} // namespace pdr::peterson

#endif // PETERSON_H
