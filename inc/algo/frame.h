#ifndef FRAME
#define FRAME

#include "z3-ext.h"
#include "stats.h"
#include "logging.h"

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
	using std::vector;
	using std::shared_ptr;
	using z3::context;
	using z3::solver;
	using z3::expr;
	using z3::expr_vector;

	using CubeSet = std::set<expr_vector, z3ext::expr_vector_less>;

	class Frame
	{
		private:
			unsigned level;
			const unsigned& max;
			context& ctx;
			Statistics& stats;
			vector<expr_vector> base_assertions;
			solver consecution_solver;
			shared_ptr<spdlog::logger> log;
			bool delta;

			CubeSet blocked_cubes; //the arguments of the clause are sorted by mic, use id to search
			//cubes that are already blocked by some cube. check the key has not been propagated, 
			//check if one of its values can.
			std::map<expr_vector, CubeSet, z3ext::expr_vector_less> subsumed; 
			// std::vector<expr_vector> blocked_cubes; //the arguments of the clause are sorted by mic, use id to search

			bool model_used = true; //used to give a warning if the SAT model is no queried before overwriting
			bool core_available = false;
			int cubes_start = 0;

			void init_solver();
		public:
			Frame(unsigned i, const unsigned& k, context& c, Statistics& s, 
					const vector<expr_vector>& assertions, shared_ptr<spdlog::logger> l, bool d = false);
			Frame(unsigned i, const unsigned& k, context& c, Statistics& s, 
					const vector<expr_vector>& assertions, bool d = false);
			
			void reset_solver();
			void reset_frame(Statistics& s, const vector<expr_vector>& assertions);

			void store_subsumed(const expr_vector& super, const expr_vector& sub);
			unsigned remove_subsumed(const expr_vector& cube);
			bool blocked(const expr_vector& cube);
			bool block_cube(const expr_vector& cube);
			bool fat_block(const expr_vector& cube);
			bool delta_block(expr_vector cube);
			expr activation_lit(unsigned level) const;

			//solver interface
			//////////////////
			bool SAT(const expr& current, expr_vector next);
			bool SAT(const expr_vector& next);
			bool fat_SAT(const expr_vector& next);
			bool delta_SAT(expr_vector next);
			bool UNSAT(const expr& current, expr_vector next);
			bool UNSAT(const expr_vector& next);

			//asserts that you do not need to use the last model
			void discard_model(); 

			// function to extract a cube representing a satisfying assignment to the last SAT call to the solver.
			// the resulting vector or expr_vector is in sorted order
			// template UnaryPredicate: function expr->bool to filter atoms from the cube. accepts 1 expr, returns bool
			template <typename UnaryPredicate> expr_vector sat_cube(UnaryPredicate p);
			template <typename UnaryPredicate> vector<expr> sat_cube_vector(UnaryPredicate p);

			// function extract the unsat_core from the solver, a subset of the assumptions
			// the resulting vector or expr_vector is in sorted order
			// assumes a core is only extracted once
			// template UnaryPredicate: function expr->bool to filter literals from the core
			// template Transform: function expr->expr. each literal is replaced by result before pushing
			expr_vector unsat_core();
			template <typename UnaryPredicate, typename Transform> 
			expr_vector unsat_core(UnaryPredicate p, Transform t);
			//////////////////////
			//end solver interface

			//Frame comparisons
			bool equals(const Frame& f) const;
			std::vector<expr_vector> diff(const Frame& f) const;

			//getters
			const CubeSet& get_blocked_cubes() const;
			bool empty() const;

			//string representations
			std::string solver_str() const;
			std::string blocked_str() const;
	};

	template <typename UnaryPredicate> 
	expr_vector Frame::sat_cube(UnaryPredicate p)
	{
		vector<expr> std_vec = sat_cube_vector(p);
		expr_vector v(std_vec[0].ctx());
		for (const expr& e : std_vec)
			v.push_back(e);
		return v;
	}

	template <typename UnaryPredicate> 
	vector<expr> Frame::sat_cube_vector(UnaryPredicate p)
	{
		model_used = true;
		z3::model m = consecution_solver.get_model();
		vector<expr> v; v.reserve(m.num_consts());
		for (unsigned i = 0; i < m.size(); i++)
		{
			z3::func_decl f = m[i];
			expr b_value = m.get_const_interp(f);
			expr literal(consecution_solver.ctx());
			if (b_value.is_true())
				 literal = f();
			else if (b_value.is_false())
				 literal = !f();
			else throw std::runtime_error("model contains non-constant");
			
			if (p(f()) == true) 
				v.push_back(literal);
		}
		std::sort(v.begin(), v.end(), z3ext::expr_less());
		return v;
	}

	template <typename UnaryPredicate, typename Transform> 
	expr_vector Frame::unsat_core(UnaryPredicate p, Transform t) 
	{ 
		expr_vector full_core = unsat_core(); 

		if (full_core.size() == 0)
			return full_core;

		vector<expr> core; core.reserve(full_core.size());
		for (const expr& e : full_core)
			if (p(e))
				core.push_back(t(e));
		std::sort(core.begin(), core.end(), z3ext::expr_less());
		return z3ext::convert(core);
	}
}
		
#endif //FRAME
