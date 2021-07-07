#ifndef FRAME
#define FRAME

#include <z3++.h>
#include <memory>
#include <unordered_set>
#include <vector>

using std::shared_ptr;
using z3::context;
using z3::solver;
using z3::expr;
using z3::expr_vector;

class Frame
{
	private:
		int level;
		shared_ptr<context> ctx;
		solver consecution_solver;

		std::unordered_set<int> added_clauses; //the arguments of the clause are sorted by mic, use id to search

		bool model_used = true; //used to give a warning if the SAT model is no queried before overwriting

	public:
		Frame(int k, shared_ptr<context> c, const std::vector<expr_vector>& assertions) : level(k), ctx(c), consecution_solver(*c)
		{
			consecution_solver.set("sat.cardinality.solver", true);
			for (const expr_vector& v : assertions)
				consecution_solver.add(v);
		}
		
		bool blocked(const expr& clause) const { return added_clauses.find(clause.id()) != added_clauses.end(); }

		bool block_cube(const expr_vector& cube)
		{
			expr clause = z3::mk_or(cube);
			bool inserted = added_clauses.insert(clause.id()).second;

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
			if (consecution_solver.check(next))
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
			
		//extracts a cube representing a satisfying assignment to the last SAT call to the solver.
		expr_vector sat_cube()
		{
			model_used = true;
			z3::model m = consecution_solver.get_model();
			expr_vector cube(*ctx);
			for (unsigned i = 0; i < m.num_consts(); i++)
				cube.push_back(m.get_const_interp(m.get_const_decl(i)));

			return cube;
		}
};

#endif //FRAME
