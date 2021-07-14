#ifndef FRAME
#define FRAME

#include <stdexcept>
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <memory>
#include <set>
#include <vector>
#include <stdexcept>
#include <numeric>
#include <z3++.h>
#include <fmt/format.h>

#include "z3-ext.h"

using std::shared_ptr;
using std::vector;
using z3::context;
using z3::solver;
using z3::expr;
using z3::expr_vector;
using Z3extensions::expr_less;
using Z3extensions::negate;

class Frame
{
	private:
		int level;
		shared_ptr<context> ctx;
		solver consecution_solver;

		std::set<expr, expr_less> blocked_cubes; //the arguments of the clause are sorted by mic, use id to search

		bool model_used = true; //used to give a warning if the SAT model is no queried before overwriting
		int cubes_start = 0;

	public:
		Frame(int k, shared_ptr<context> c, const std::vector<expr_vector>& assertions) : level(k), ctx(c), consecution_solver(*c/*, "QF_FD"*/)
		{
			consecution_solver.set("sat.cardinality.solver", true);
			consecution_solver.set("cardinality.solver", true);
			// consecution_solver.set("lookahead_simplify", true);
			for (const expr_vector& v : assertions)
				consecution_solver.add(v);

			cubes_start = std::accumulate(assertions.begin(), assertions.end(), 
					0, [](int agg, const expr_vector& v) { return agg + v.size(); });
		}
		
		bool blocked(const expr& cube) const { return blocked_cubes.find(cube) != blocked_cubes.end(); }

		bool block_cube(const expr_vector& cube)
		{
			expr clause = z3::mk_or(negate(cube));
			bool inserted = blocked_cubes.insert(z3::mk_and(cube)).second;

			if (!inserted) //TODO possibly check some subsumption here
				return false;

			consecution_solver.add(clause);
			return true;
		}

		bool SAT(const expr& current, expr_vector next)
		{
			next.push_back(current);
			return SAT(next);
		}
		bool SAT(const expr_vector& next) 
		{ 
			if (consecution_solver.check(next) == z3::sat)
			{
				if(!model_used)
					std::cerr << "PDR::WARNING: last SAT model unused and discarded" << std::endl;
				model_used = false;
				return true;
			}
			return false;
		}

		bool UNSAT(const expr& current, expr_vector next) 
		{ 
			next.push_back(current);
			return !SAT(next);
		}
		bool UNSAT(const expr_vector& next) { return !SAT(next); }
			
		// function to extract a cube representing a satisfying assignment to the last SAT call to the solver.
		template <typename Vec>
		void sat_cube(Vec& v)
		{
			model_used = true;
			z3::model m = consecution_solver.get_model();
			for (unsigned i = 0; i < m.num_consts(); i++)
				v.push_back(m.get_const_interp(m.get_const_decl(i)));
		}

		// all atoms (that satisfy p) are stored in v
		template <typename Vec, typename UnaryPredicate>
		void sat_cube(Vec& v, UnaryPredicate p)
		{
			model_used = true;
			z3::model m = consecution_solver.get_model();
			for (unsigned i = 0; i < m.size(); i++)
			{
				z3::func_decl f = m[i];
				expr b_value = m.get_const_interp(f);
				expr literal(*ctx);
				if (b_value.is_true())
					 literal = f();
				else if (b_value.is_false())
					 literal = !f();
				else throw std::runtime_error("model contains non-constant");
				
				if (p(f()) == true) 
					v.push_back(literal);
			}
		}

		expr_vector unsat_core() const { return consecution_solver.unsat_core(); }
		template <typename UnaryPredicate, typename Transform>
		expr_vector unsat_core(UnaryPredicate p, Transform t) const 
		{ 
			expr_vector full_core = consecution_solver.unsat_core(); 
			if (full_core.size() == 0)
				return full_core;

			expr_vector core(full_core[0].ctx());
			for (const expr& e : full_core)
				if (p(e))
					core.push_back(t(e));
			return core;
		}
		
		//does nothing bet asserts that you do not need to use the last model
		void discard_model() { model_used = true; }

		template <typename Vec, typename UnaryPredicate, typename VecReserve>
		void sat_cube(Vec& v, UnaryPredicate p, VecReserve reserve)
		{
			model_used = true;
			z3::model m = consecution_solver.get_model();
			reserve(m.num_consts());
			for (unsigned i = 0; i < m.num_consts(); i++)
			{
				expr e = m.get_const_interp(m.get_const_decl(i));
				if (p(e) == true) 
					v.push_back(e);
			}
		}

		bool equals(const Frame& f) const { return blocked_cubes == f.blocked_cubes; }

		std::vector<expr> diff(const Frame& f) const
		{
			std::vector<expr> out;
			std::set_difference(
					blocked_cubes.begin(), blocked_cubes.end(),
					f.blocked_cubes.begin(), f.blocked_cubes.end(),
					std::back_inserter(out), expr_less());
			return out;
		}

		std::string solver_str() const
		{
			std::string str(fmt::format("solver level {}\n", level));
			const expr_vector& asserts = consecution_solver.assertions();
			
			auto it = asserts.begin();
			for (int i = 0; i < cubes_start && it != asserts.end(); i++) it++;

			for (; it != asserts.end(); it++)
				str += fmt::format("- {}\n", (*it).to_string());

			return str;
		}

		std::string blocked_str() const
		{
			std::string str(fmt::format("blocked cubes level {}\n", level));
			for (const expr& e : blocked_cubes)
				str += fmt::format("- {}\n", e.to_string());

			return str;
		}
};

#endif //FRAME
