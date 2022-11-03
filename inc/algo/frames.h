#ifndef FRAMES
#define FRAMES

#include "_logging.h"
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
   private:
    Context& ctx;
    IModel& model;
    Logger& logger;
    const bool LOG_SAT_CALLS = false;

    std::vector<std::unique_ptr<Frame>> frames;
    Solver FI_solver;
    Solver delta_solver;
    std::vector<z3::expr> act; // activation variables for each frame

   public:
    // solver containing only the intial state
    z3::solver init_solver; // TODO immutable interface

    Frames(Context& c, IModel& m, Logger& l);

    // reset the sequence to F_0, F_1 (frontier 0)
    void reset();

    // reduce the max_pebbles constraint to 'x'
    // redo propagation for the previous level
    // return an invariant level if propagation finds one
    std::optional<size_t> reuse();

    // increase the max_pebbles constraint to 'x'
    // carry over all learned cubes to F_1 in a new sequence (if valid)
    void reset_to_F1();

    // frame interface
    //
    // pops frames until the given index is the frontier
    void clear_until(size_t until_index);
    // initialize a new frame to the base
    void extend();

    // reset solvers and repopulate with current blocked cubes
    void repopulate_solvers();

    bool remove_state(const z3::expr_vector& cube, size_t level);
    bool delta_remove_state(const z3::expr_vector& cube, size_t level);
    std::optional<size_t> propagate();
    std::optional<size_t> propagate(size_t k);
    void push_forward_delta(size_t level, bool repeat = false);

    // returns if the clause of the given cube is inductive relative to F_frame
    bool inductive(const std::vector<z3::expr>& cube, size_t frame);
    bool inductive(const z3::expr_vector& cube, size_t frame);
    std::optional<z3::expr_vector> counter_to_inductiveness(
        const std::vector<z3::expr>& cube, size_t frame);
    std::optional<z3::expr_vector> counter_to_inductiveness(
        const z3::expr_vector& cube, size_t frame);

    // returns if there exists a transition from frame to cube,
    // allows collection of witness from solver(frame) if true.
    bool trans_source(
        size_t frame, const z3::expr_vector& dest_cube, bool primed = false);
    std::optional<z3ext::solver::Witness> get_trans_source(
        size_t frame, const z3::expr_vector& dest_cube, bool primed = false);

    // Solver calls (non-modifying)
    //
    // returns if there exists a satisfying assignment
    bool SAT(size_t frame, const z3::expr_vector& assumptions);
    bool SAT(size_t frame, z3::expr_vector&& assumptions);

    const z3::model get_model(size_t frame) const;
    void reset_solver(size_t frame);

    // getters (non-modifying)
    //
    size_t frontier() const;
    Solver& get_solver(size_t frame);
    const Solver& get_solver(size_t frame) const;
    const Frame& operator[](size_t i);
    // returns all cubes blocked in Frame 1. adjusted for delta encoding.
    z3ext::CubeSet get_blocked_in(size_t i) const;

    void log_blocked() const;
    void log_solver(bool clauses_only) const;
    std::string blocked_str() const;
    std::string solver_str(bool clauses_only) const;
  };

} // namespace pdr

#endif // FRAMES
