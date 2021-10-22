#ifndef SOLVER
#define SOLVER
#include "z3-ext.h"

#include <fmt/core.h>
#include <memory>
#include <numeric>
#include <vector>
#include <z3++.h>

namespace pdr
{
	class Solver
	{
		private:	
			z3::context& ctx;
			z3::solver internal_solver;

			std::vector<z3::expr_vector> base_assertions;

			bool model_used = true; //used to give a warning if the SAT model is no queried before overwriting
			bool core_available = false;
			unsigned cubes_start; //point where base_assertions ends and other assertions begin

		public:
			Solver(z3::context& c, std::vector<z3::expr_vector> base); 

			void init();
			void reset();
			void reset(std::vector<z3::expr_vector> assertions);
			void block(const z3::expr_vector& cube);
			void block(const z3::expr_vector& cube, const z3::expr& act);
			void add(const z3::expr& e);

			bool SAT(const z3::expr_vector& assumptions);
			z3::model get_model() const;
			void discard_model();
			std::string as_str(const std::string& header = "") const;

			// function to extract a cube representing a satisfying assignment to the last SAT call to the solver.
			// the resulting vector or expr_vector is in sorted order
			// template UnaryPredicate: function expr->bool to filter atoms. accepts 1 expr, returns bool
			template <typename UnaryPredicate> z3::expr_vector witness(UnaryPredicate p);
			template <typename UnaryPredicate> std::vector<z3::expr> witness_vector(UnaryPredicate p);

			// function extract the unsat_core from the solver, a subset of the assumptions
			// the resulting vector or expr_vector is in sorted order
			// assumes a core is only extracted once
			// template UnaryPredicate: function expr->bool to filter literals from the core
			// template Transform: function expr->expr. each literal is replaced by result before pushing
			z3::expr_vector unsat_core();
			template <typename UnaryPredicate, typename Transform> 
			z3::expr_vector unsat_core(UnaryPredicate p, Transform t);
	};

	template <typename UnaryPredicate> 
	z3::expr_vector Solver::witness(UnaryPredicate p)
	{
		std::vector<z3::expr> std_vec = witness_vector(p);
		z3::expr_vector v(std_vec[0].ctx());
		for (const z3::expr& e : std_vec)
			v.push_back(e);
		return v;
	}

	template <typename UnaryPredicate> 
	std::vector<z3::expr> Solver::witness_vector(UnaryPredicate p)
	{
		model_used = true;
		z3::model m = internal_solver.get_model();
		std::vector<z3::expr> v; v.reserve(m.num_consts());
		for (unsigned i = 0; i < m.size(); i++)
		{
			z3::func_decl f = m[i];
			z3::expr b_value = m.get_const_interp(f);
			z3::expr literal(internal_solver.ctx());
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
	z3::expr_vector Solver::unsat_core(UnaryPredicate p, Transform t) 
	{ 
		z3::expr_vector full_core = unsat_core(); 

		if (full_core.size() == 0)
			return full_core;

		std::vector<z3::expr> core; core.reserve(full_core.size());
		for (const z3::expr& e : full_core)
			if (p(e))
				core.push_back(t(e));
		std::sort(core.begin(), core.end(), z3ext::expr_less());
		return z3ext::convert(core);
	}
}
#endif //SOLVER
