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
  using CubeSet = std::set<z3::expr_vector, z3ext::expr_vector_less>;

  class Frames
  {
   private:
    Context& ctx;
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

    Frames(Context& c, Logger& l);
    // frame interface
    //
    //
    void clear_until(size_t until_index = 0);
    void extend();
    void reset_constraint(unsigned x);
    void repopulate_solvers();
    // assumes:
    // - a run of PDR has finished
    // - base assertions have been changed and are a superset of the previous
    // copy all old cubes that are not reachable from I into a new F_1
    void increment_reset(unsigned x);
    bool remove_state(const z3::expr_vector& cube, size_t level);
    bool delta_remove_state(const z3::expr_vector& cube, size_t level);
    bool fat_remove_state(const z3::expr_vector& cube, size_t level);
    int propagate(bool repeat = false);
    void push_forward_delta(unsigned level, bool repeat = false);
    int push_forward_fat(unsigned level, bool repeat = false);

    // queries
    //
    bool init_implies(const z3::expr_vector& formula) const;
    // returns if the clause of the given cube is inductive relative to F_frame
    bool inductive(const std::vector<z3::expr>& cube, size_t frame) const;
    bool inductive(const z3::expr_vector& cube, size_t frame) const;
    std::optional<z3::expr_vector>
        counter_to_inductiveness(const std::vector<z3::expr>& cube,
                                 size_t frame) const;
    std::optional<z3::expr_vector>
        counter_to_inductiveness(const z3::expr_vector& cube,
                                 size_t frame) const;
    // returns if there exists a transition from frame to cube,
    // allows collection of witness from solver(frame) if true.
    bool trans_source(size_t frame, const z3::expr_vector& dest_cube,
                      bool primed = false) const;
    std::optional<z3::expr_vector>
        get_trans_source(size_t frame, const z3::expr_vector& dest_cube,
                         bool primed = false) const;

    // Solver calls
    //
    // returns if there exists a satisfying assignment
    bool SAT(size_t frame, const z3::expr_vector& assumptions) const;
    bool SAT(size_t frame, z3::expr_vector&& assumptions) const;

    const z3::model get_model(size_t frame) const;
    void reset_solver(size_t frame);

    // getters
    //
    unsigned frontier() const;
    Solver& get_solver(size_t frame) const;
    const Solver& get_const_solver(size_t frame) const;
    const Frame& operator[](size_t i);
    // returns all cubes blocked in Frame 1. adjusted for delta encoding.
    CubeSet get_blocked(size_t i) const;

    void log_solvers() const;
    std::string blocked_str() const;
    std::string solvers_str() const;
  };

} // namespace pdr

#endif // FRAMES
