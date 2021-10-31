#ifndef FRAMES
#define FRAMES

#include "_logging.h"
#include "frame.h"
#include "logger.h"
#include "pdr-model.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <cstddef>
#include <fmt/format.h>
#include <memory>
#include <set>
#include <vector>
#include <z3++.h>

namespace pdr
{
    using CubeSet = std::set<z3::expr_vector, z3ext::expr_vector_less>;
    using Witness = std::unique_ptr<z3::model>;

    class Frames
    {
      private:
        bool delta;
        z3::context& ctx;
        const PDRModel& model;
        Logger& logger;
        std::vector<z3::expr_vector> base_assertions;
        std::unique_ptr<Solver> delta_solver;
        std::vector<std::unique_ptr<Frame>> frames;
        std::vector<z3::expr> act;

      public:
        z3::solver init_solver;

        Frames(bool d, z3::context& c, const PDRModel& m, Logger& l);

        // frame interface
        //
        void extend();
        void reset_frames(Statistics& s,
                          const std::vector<z3::expr_vector>& assertions);
		void clean_solvers();
        bool remove_state(const z3::expr_vector& cube, size_t level);
        bool delta_remove_state(const z3::expr_vector& cube, size_t level);
        bool fat_remove_state(const z3::expr_vector& cube, size_t level);
        bool propagate(unsigned level, bool repeat = false);
        void push_forward_delta(unsigned level, bool repeat = false);
        bool push_forward_fat(unsigned level, bool repeat = false);

        // queries
        //
        bool init_implies(const z3::expr_vector& formula) const;
        // returns if the negation of cube is inductive relative to F_frame
        bool inductive(const std::vector<z3::expr>& cube, size_t frame) const;
        bool inductive(const z3::expr_vector& cube, size_t frame) const;
        Witness counter_to_inductiveness(const std::vector<z3::expr>& cube,
                                         size_t frame) const;
        Witness counter_to_inductiveness(const z3::expr_vector& cube,
                                         size_t frame) const;
        // returns if there exists a transition from frame to cube,
        // allows collection of witness from solver(frame) if true.
        bool trans_from_to(size_t frame, const z3::expr_vector& cube,
                           bool primed = false) const;
        std::unique_ptr<z3::model>
            get_trans_from_to(size_t frame, const z3::expr_vector& cube,
                              bool primed = false) const;

        // Solver calls
        //
        // returns if there exists a satisfying assignment
        bool SAT(size_t frame, const z3::expr_vector& assumptions) const;
        bool SAT(size_t frame, z3::expr_vector&& assumptions) const;
        // returns a satisfying assignment if it exists
        Witness SAT_model(size_t frame,
                          const z3::expr_vector& assumptions) const;
        Witness SAT_model(size_t frame, z3::expr_vector&& assumptions) const;
        const z3::model get_model(size_t frame) const;
        void reset_solver(size_t frame);

        // getters
        //
        unsigned frontier() const;
        Solver* solver(size_t frame);
        const Frame& operator[](size_t i);

        void log_solvers() const;
        std::string blocked_str() const;
        std::string solvers_str() const;
    };

} // namespace pdr

#endif // FRAMES
