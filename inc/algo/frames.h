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
    void extend();

    // reset the sequence to F_0, F_1 (frontier 0)
    void reset();

    // pops frames until the given index is the frontier
    void clear_until(size_t until_index);

    // reset solvers and repopulate with current blocked cubes
    void repopulate_solvers();

    // relaxing opdr functions
    //
    // carry over all learned cubes to F_1 in a new sequence (if valid)
    // used after a constraint has been relaxed since a previous model
    void copy_to_F1();
    // carry over all learned cubes to a new sequence F_1..F_k (if valid)
    // used after a constraint has been relaxed since a previous model
    void copy_to_Fk();
    // carry over all learned cubes to a new sequence F_1..F_k.
    // if a cube is no longer valid under the new system: add a constraint
    // specifying it for the old system.
    // @param old_step: the given size of the constraint from the previous run
    // @param old_constraint: clauses that describe the constraint from the
    // previous run
    void copy_to_Fk_keep(
        size_t old_step, z3::expr_vector const& old_constraint);

    // constraining ipdr functions
    //
    // redo propagation for the previous level
    // return an invariant level if propagation finds one
    // used after a constraint has been tightened
    std::optional<size_t> reuse();

    // Raw solver queries
    //
    // returns if there exists a satisfying assignment
    bool SAT(size_t frame, const z3::expr_vector& assumptions);
    bool SAT(size_t frame, z3::expr_vector&& assumptions);

    // state removal functions
    //
    //
    bool remove_state(const std::vector<z3::expr>& cube, size_t level);
    // removes a state and handles subsumption with cube constrained.
    // slower that regular remove state. used only during relaxation
    bool remove_state_constrained(
        const std::vector<z3::expr>& cube, size_t level);
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
    bool trans_source(size_t frame,
        const std::vector<z3::expr>& dest_cube,
        bool primed = false);
    // returns the witness to a transition if it exists, else none
    std::optional<z3ext::solver::Witness> get_trans_source(size_t frame,
        const std::vector<z3::expr>& dest_cube,
        bool primed = false);

    // returns true if the given cube or a stronger cube is already blocked
    // at level
    std::optional<size_t> already_blocked(
        std::vector<z3::expr> const& cube, size_t level) const;

    // getters
    //
    // the maximum k for which F_1...F_k describes reachable states in i steps
    // k = |frames|-2 (second-to-last) frame in regular pdr
    // if there is a detached_frontier, it may be of a lesser index
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

    std::vector<Frame> frames;
    // default frontier = |frames| - 2 (second-to-last frame)
    // override allowing more frames to exist (for relaxing pdr)
    std::optional<unsigned> detached_frontier;

    Solver FI_solver;
    Solver delta_solver;
    // activation variables for each frame. if present in a query, the clauses
    // from the corresponding frame are loaded
    std::vector<z3::expr> act;
    // Defined in the solver as: clit[i] <=> constraint(i),
    // there is a constraint for each pdr-run after which a relaxing step was
    // made.
    // each constraint is user-defined such that:
    // constraint[i] => constraint[j] if i < j
    // maps: i -> constraint[i]
    std::map<size_t, z3::expr_vector> constraints;
    // activation literals for incremental relaxing.
    // maps: i -> clit[i]
    std::map<size_t, z3::expr> clits;
    // stored for easy detection in z3ext::mk_constrained_cube()
    // clit[i].id() -> i
    std::map<unsigned, size_t> clit_ids;

    void new_constraint(size_t i, z3::expr_vector const& clauses);

    void init_frames();
    void new_frame();
    void refresh_solver_if_clogged();
    // define each of the "constraints" in a logic formula:
    // expr(__constraint{i}__) <=> constraint[i]
    z3::expr_vector old_constraints() const;

    // performs state removal for the delta-encoding. called by remove_state().
    //
    // @pre: "cube" is unreachable within "level" steps of the system
    // @post: "cube" is marked as unreachble in "frames[level]" in the
    // delta-encoding and "cube" has been blocked at "level" in the
    // "delta_solver".
    // @return: true if "cube" was newly removed, false if it was already.
    bool delta_remove_state(const std::vector<z3::expr>& cube, size_t level);
    // state removal for the delta-encoding with constrained cubes.
    // called by remove_state_constrained().
    bool delta_remove_state_constrained(
        const std::vector<z3::expr>& cube, size_t level);
  };

} // namespace pdr

#endif // FRAMES
