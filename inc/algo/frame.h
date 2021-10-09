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
			bool delta;
			unsigned level;
			Logger& logger;
			vector<z3::expr_vector> base_assertions;
			std::unique_ptr<Solver> solver;

			CubeSet blocked_cubes; //the arguments of the clause are sorted by mic, use id to search
			//cubes that are already blocked by some cube. check the key has not been propagated, 
			//check if one of its values can.
			std::map<z3::expr_vector, CubeSet, z3ext::expr_vector_less> subsumed; 
			// std::vector<expr_vector> blocked_cubes; //the arguments of the clause are sorted by mic, use id to search
			void init_solver();
		public:
			Frame(bool d, unsigned i, z3::context& ctx, Logger& l, const vector<z3::expr_vector>& assertions = {});
			
			void reset_solver();
			void reset_frame(Statistics& s, const vector<z3::expr_vector>& assertions);

			void store_subsumed(const z3::expr_vector& super, const z3::expr_vector& sub);
			unsigned remove_subsumed(const z3::expr_vector& cube);
			bool blocked(const z3::expr_vector& cube);
			bool block_cube(const z3::expr_vector& cube);
			bool fat_block(const z3::expr_vector& cube);
			bool delta_block(z3::expr_vector cube);
			z3::expr activation_lit(unsigned level) const;

			//Frame comparisons
			bool equals(const Frame& f) const;
			std::vector<z3::expr_vector> diff(const Frame& f) const;

			//getters
			const CubeSet& get_blocked_cubes() const;
			bool empty() const;
			bool is_delta() const;
			Solver* get_solver() const;

			//string representations
			std::string blocked_str() const;
	};
}
		
#endif //FRAME
