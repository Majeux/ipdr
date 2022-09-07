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

    z3::expr_vector frame_base;

    std::vector<std::unique_ptr<Frame>> frames;
    // in delta encoding, all frames are managed by a single solver
    std::unique_ptr<Solver> delta_solver;
    std::vector<z3::expr> act; // activation variables for each frame

    // prepare the sequence { F_0 } from an empty {} sequence
    void init_frame_I();

   public:
    // solver containing only the intial state
    z3::solver init_solver; // TODO non-mutable interface

    // cardinality constraint on the maximum allowed marked literals in a state
    std::optional<unsigned> max_pebbles;

    Frames(Context& c, IModel& m, Logger& l);
    Frames(Context& c, IModel& m, Logger& l, std::optional<unsigned> constraint);

    // reset the sequence to F_0, F_1 (frontier 0)
    void reset();

    // reset the sequence to F_0, F_1 (frontier 0)
    // and set 'new_constraint' as the new 'max_pebbles'
    void reset(std::optional<unsigned> new_constraint);

    // reduce the max_pebbles constraint to 'x'
    // redo propagation for the previous level
    // return an invariant level if propagation finds one
    std::optional<size_t> decrement_reset(unsigned x);

    // increase the max_pebbles constraint to 'x'
    // carry over all learned cubes to F_1 in a new sequence (if valid)
    void increment_reset(unsigned x);

    // frame interface
    //
    // pops frames until the given index is the frontier
    void clear_until(size_t until_index);
    void extend();

    // reset solvers and repopulate with current blocked cubes
    void repopulate_solvers();

    bool remove_state(const z3::expr_vector& cube, size_t level);
    bool delta_remove_state(const z3::expr_vector& cube, size_t level);
    bool fat_remove_state(const z3::expr_vector& cube, size_t level);
    std::optional<size_t> propagate();
    std::optional<size_t> propagate(size_t k);
    void push_forward_delta(size_t level, bool repeat = false);
    std::optional<size_t> push_forward_fat(size_t level, bool repeat = false);

    // queries (non-modifying)
    //
    bool I_implies(const z3::expr_vector& formula) const;
    // returns if the clause of the given cube is inductive relative to F_frame
    bool inductive(const std::vector<z3::expr>& cube, size_t frame) const;
    bool inductive(const z3::expr_vector& cube, size_t frame) const;
    std::optional<z3::expr_vector> counter_to_inductiveness(
        const std::vector<z3::expr>& cube, size_t frame) const;
    std::optional<z3::expr_vector> counter_to_inductiveness(
        const z3::expr_vector& cube, size_t frame) const;
    // returns if there exists a transition from frame to cube,
    // allows collection of witness from solver(frame) if true.
    bool trans_source(size_t frame, const z3::expr_vector& dest_cube,
        bool primed = false) const;
    std::optional<z3::expr_vector> get_trans_source(size_t frame,
        const z3::expr_vector& dest_cube, bool primed = false) const;

    // Solver calls (non-modifying)
    //
    // returns if there exists a satisfying assignment
    bool SAT(size_t frame, const z3::expr_vector& assumptions) const;
    bool SAT(size_t frame, z3::expr_vector&& assumptions) const;

    const z3::model get_model(size_t frame) const;
    void reset_solver(size_t frame);

    // getters (non-modifying)
    //
    size_t frontier() const;
    Solver& get_solver(size_t frame) const;
    const Solver& get_const_solver(size_t frame) const;
    const Frame& operator[](size_t i);
    // returns all cubes blocked in Frame 1. adjusted for delta encoding.
    z3ext::CubeSet get_blocked(size_t i) const;

    void log_solvers(bool clauses_only) const;
    std::string blocked_str() const;
    std::string solvers_str(bool clauses_only) const;
  };

} // namespace pdr

#endif // FRAMES
