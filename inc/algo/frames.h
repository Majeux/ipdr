#ifndef FRAMES
#define FRAMES

#include "frame.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <cstddef>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <set>
#include <vector>
#include <z3++.h>

namespace pdr
{

  class Frames
  {
   public:
    // solver containing only the intial state
    z3::solver init_solver; // TODO immutable interface

    Frames(Context c, IModel& m, Logger& l);

    // sequence manipulation functions
    //
    // initialize a new frame to the base
    void extend();

    // reset the sequence to F_0, F_1 (frontier 0)
    void reset();

    // increase the max_pebbles constraint to 'x'
    // carry over all learned cubes to F_1 in a new sequence (if valid)
    void reset_to_F1();

    // reduce the max_pebbles constraint to 'x'
    // redo propagation for the previous level
    // return an invariant level if propagation finds one
    std::optional<size_t> reuse();

    // pops frames until the given index is the frontier
    void clear_until(size_t until_index);

    // reset solvers and repopulate with current blocked cubes
    void repopulate_solvers();

    // state removal functions
    //
    bool remove_state(const std::vector<z3::expr>& cube, size_t level);
    bool delta_remove_state(const std::vector<z3::expr>& cube, size_t level);
    std::optional<size_t> propagate();
    std::optional<size_t> propagate(size_t k);
    void push_forward_delta(size_t level, bool repeat = false);

    // query functions over the state space the frames represent
    //
    // returns true if the negation of cube is inductive relative to F_frame
    bool inductive(const std::vector<z3::expr>& cube, size_t frame);
    // returns a cube in `F_frame \cup !cube` that leads to a cube-state
    std::optional<std::vector<z3::expr>> counter_to_inductiveness(
        const std::vector<z3::expr>& cube, size_t frame);

    // returns if there exists a transition from frame to cube,
    // allows collection of witness from solver(frame) if true.
    // if primed: cube is already in next state, else first convert it
    bool trans_source(size_t frame, const std::vector<z3::expr>& dest_cube,
        bool primed = false);
    // returns the witness to a transition if it exists, else none
    std::optional<z3ext::solver::Witness> get_trans_source(size_t frame,
        const std::vector<z3::expr>& dest_cube, bool primed = false);

    // getters
    //
    size_t frontier() const;
    // const z3::model get_model(size_t frame) const;
    Solver& get_solver(size_t frame);
    const Solver& get_solver(size_t frame) const;
    const Frame& operator[](size_t i);
    // returns all cubes blocked in Frame i. adjusted for delta encoding.
    z3ext::CubeSet get_blocked_in(size_t i) const;

    // logging and output
    // 
    void log_blocked() const;
    void log_solver(bool clauses_only) const;
    std::string blocked_str() const;
    std::string solver_str(bool clauses_only) const;

   private:
    Context ctx;
    IModel& model;
    Logger& log;
    const bool LOG_SAT_CALLS = false;
    bool do_repopulate = true;

    std::vector<Frame> frames;
    Solver FI_solver;
    Solver delta_solver;
    std::vector<z3::expr> act; // activation variables for each frame

    void refresh_solver_if_clogged();

    // Raw solver queries
    //
    // returns if there exists a satisfying assignment
    bool SAT(size_t frame, const z3::expr_vector& assumptions);
    bool SAT(size_t frame, z3::expr_vector&& assumptions);


  };

} // namespace pdr

#endif // FRAMES
