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
			Frames(bool d, z3::context& c, const PDRModel& m, Logger& l);
			void extend();

			void remove_state(expr_vector& cube, size_t level);
			void delta_remove_state(expr_vector& cube, size_t level);
			void fat_remove_state(expr_vector& cube, size_t level);

			//queries
			bool inductive_rel_to(const z3::expr_vector& cube, size_t frame) const;
			bool transition_from_to(size_t frame, const z3::expr_vector& cube) const;

			//SAT calls
			bool SAT(size_t frame, z3::expr_vector& assumptions) const;
			void discard_model(size_t frame);

			//getters
			unsigned frontier() const;
	};
	
}
		
#endif //FRAMES
