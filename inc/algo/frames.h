#ifndef FRAMES
#define FRAMES
#include "frame.h"
#include "logging.h"
#include "logger.h"
#include "pdr-model.h"
#include "stats.h"
#include "solver.h"
#include "z3-ext.h"

#include <cstddef>
#include <memory>
#include <set>
#include <vector>
#include <z3++.h>
#include <fmt/format.h>

namespace pdr 
{
	using CubeSet = std::set<z3::expr_vector, z3ext::expr_vector_less>;

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

			//frame interface
			//
			void extend();
			bool remove_state(const expr_vector& cube, size_t level);
			bool delta_remove_state(const expr_vector& cube, size_t level);
			bool fat_remove_state(const expr_vector& cube, size_t level);
			bool propagate(unsigned level, bool repeat = false);
			void push_forward_delta(unsigned level, bool repeat = false);
			bool push_forward_fat(unsigned level, bool repeat = false);

			//queries
			//
			bool init_implies(const expr_vector& formula) const;
			//returns if the negation of cube is inductive relative to frame
			bool neg_inductive_rel_to(const z3::expr_vector& cube, size_t frame) const;
			//returns if there exists a transition from frame to cube, 
			//allows collection of witness from solver(frame) if true.
			bool transition_from_to(size_t frame, const z3::expr_vector& cube, bool primed = false) const;

			//Solver calls
			//
			bool SAT(size_t frame, const z3::expr_vector& assumptions) const;
			bool SAT(size_t frame, z3::expr_vector& assumptions) const;
			void discard_model(size_t frame);
			void reset_solver(size_t frame);

			//getters
			//
			unsigned frontier() const;
			Solver* solver(size_t frame);
			const Frame& operator[](size_t i);

			void log_solvers() const;
	};
	
}
		
#endif //FRAMES
