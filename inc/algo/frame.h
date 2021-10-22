#ifndef FRAME
#define FRAME

#include "z3-ext.h"
#include "logging.h"
#include "logger.h"
#include "solver.h"
#include "stats.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <z3++.h>
#include <fmt/format.h>

namespace pdr 
{
	using CubeSet = std::set<z3::expr_vector, z3ext::expr_vector_less>;

	class Frame
	{
		private:
			unsigned level;
			Logger& logger;
			std::unique_ptr<Solver> solver;
			CubeSet blocked_cubes; //the arguments of the clause are sorted by mic, use id to search

			void init_solver();
		public:
			//Delta frame, without logger
			Frame(unsigned i, Logger& l);
			//Fat frame, with its own logger
			Frame(unsigned i, z3::context& c, const std::vector<z3::expr_vector>& assertions, Logger& l);
			
			void reset_solver(const std::vector<z3::expr_vector>& assertions);
			void reset_frame(Statistics& s, const vector<z3::expr_vector>& assertions);

			void store_subsumed(const z3::expr_vector& super, const z3::expr_vector& sub);
			unsigned remove_subsumed(const z3::expr_vector& cube);
			bool blocked(const z3::expr_vector& cube);
			bool block(const z3::expr_vector& cube);
			void block_in_solver(const z3::expr_vector& cube);

			//Frame comparisons
			bool equals(const Frame& f) const;
			std::vector<z3::expr_vector> diff(const Frame& f) const;

			//getters
			const CubeSet& get_blocked() const;
			bool empty() const;
			Solver* get_solver() const;

			//string representations
			std::string blocked_str() const;
	};
}
		
#endif //FRAME
