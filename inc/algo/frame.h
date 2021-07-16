#ifndef FRAME
#define FRAME

#include <stdexcept>
#include <string>
#include <memory>
#include <set>
#include <vector>
#include <z3++.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "z3-ext.h"
#include "stats.h"

using std::vector;
using z3::context;
using z3::solver;
using z3::expr;
using z3::expr_vector;

class Frame
{
	private:
		int level;
		Statistics& stats;
		solver consecution_solver;

		std::set<expr, z3ext::expr_less> blocked_cubes; //the arguments of the clause are sorted by mic, use id to search

		bool model_used = true; //used to give a warning if the SAT model is no queried before overwriting
		int cubes_start = 0;

	public:
		Frame(int k, context& c, Statistics& s, const std::vector<expr_vector>& assertions);
		
		bool blocked(const expr& cube) const;
		bool block_cube(const expr_vector& cube);

		//solver interface
		//////////////////
		bool SAT(const expr& current, expr_vector next);
		bool SAT(const expr_vector& next);
		bool UNSAT(const expr& current, expr_vector next);
		bool UNSAT(const expr_vector& next);

		//asserts that you do not need to use the last model
		void discard_model(); 

		// function to extract a cube representing a satisfying assignment to the last SAT call to the solver.
		// template Vec: vector-like container to store the cube in. must support .push_back(z3::expr);
		// template UnaryPredicate: function expr->bool to filter atoms from the cube. accepts 1 expr, returns bool
		// tempalte VecReserve: vector.reserve() function (or other preprocessing). executed before pushing
		template <typename Vec>	
		void sat_cube(Vec& v);
		template <typename Vec, typename UnaryPredicate> 
		void sat_cube(Vec& v, UnaryPredicate p);
		template <typename Vec, typename UnaryPredicate, typename VecReserve>

		void sat_cube(Vec& v, UnaryPredicate p, VecReserve reserve);
		// function extract the unsat_core from the solver
		// template UnaryPredicate: function expr->bool to filter literals from the core
		// template Transform: function expr->expr. each literal is replaced by result before pushing
		expr_vector unsat_core() const;
		template <typename UnaryPredicate, typename Transform> 
		expr_vector unsat_core(UnaryPredicate p, Transform t) const;
		//////////////////////
		//end solver interface

		//Frame comparisons
		bool equals(const Frame& f) const;
		std::vector<expr> diff(const Frame& f) const;

		//string representations
		std::string solver_str() const;
		std::string blocked_str() const;
};

#include "frame-temp.h"

#endif //FRAME
