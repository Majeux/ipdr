#include "frame.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <chrono>
#include <z3++.h>


namespace pdr
{
	using z3ext::join_expr_vec;

	Frame::Frame(int k, context& c, Statistics& s, const vector<expr_vector>& assertions, shared_ptr<spdlog::logger> l) 
		: level(k), stats(s), consecution_solver(c/*, "QF_FD"*/), log(l)
	{
		consecution_solver.set("sat.cardinality.solver", true);
		consecution_solver.set("cardinality.solver", true);
		// consecution_solver.set("lookahead_simplify", true);
		for (const expr_vector& v : assertions)
			consecution_solver.add(v);

		cubes_start = std::accumulate(assertions.begin(), assertions.end(), 
				0, [](int agg, const expr_vector& v) { return agg + v.size(); });
	}

	Frame::Frame(int k, context& c, Statistics& s, const vector<expr_vector>& assertions) 
		: Frame(k, c, s, assertions, shared_ptr<spdlog::logger>()) 
	{ }

	bool Frame::blocked(const expr_vector& cube) const 
	{ 
		for (const expr_vector& blocked_cube : blocked_cubes)
		{
			if (z3ext::subsumes(blocked_cube, cube))
			{
				return true; //equal or stronger clause found
			}
		}
		return false;	
	}
	
	void Frame::remove_subsumed(const expr_vector& cube)
	{
		auto new_end = std::remove_if(blocked_cubes.begin(), blocked_cubes.end(),
				[&cube](const expr_vector& blocked) { return z3ext::subsumes(cube, blocked); });
		blocked_cubes.erase(new_end, blocked_cubes.end());
	}

	//cube is sorted by id()
	//block cube unless it, or a stronger version, is already blocked
	bool Frame::block_cube(const expr_vector& cube)
	{
		if (blocked(cube)) //do not add if an equal or stronger version is already blocked
			return false;

		remove_subsumed(cube); //remove all blocked cubes that are weaker than cube

		blocked_cubes.push_back(cube);

		expr clause = z3::mk_or(z3ext::negate(cube));
		consecution_solver.add(clause);
		return true;
	}

	bool Frame::SAT(const expr& current, expr_vector next)
	{
		next.push_back(current);
		return SAT(next);
	}

	bool Frame::SAT(const expr_vector& next) 
	{ 
		using std::chrono::steady_clock; 
		auto start = steady_clock::now();
		bool result;

		if (consecution_solver.check(next) == z3::sat)
		{
			if(!model_used)
				std::cerr << "PDR::WARNING: last SAT model unused and discarded" << std::endl;
			model_used = false;
			result = true;
		}
		else
			result = false;

		std::chrono::duration<double> diff(steady_clock::now() - start);
		stats.solver_call(level, diff.count());
		return result;
	}

	bool Frame::UNSAT(const expr& current, expr_vector next) 
	{ 
		next.push_back(current);
		return !SAT(next);
	}

	bool Frame::UNSAT(const expr_vector& next) 
	{ 
		return !SAT(next);
	}

	void Frame::discard_model()
	{
		model_used = true;
	}

	bool Frame::equals(const Frame& f) const
	{ 
		return blocked_cubes == f.blocked_cubes;
	}

	std::vector<expr_vector> Frame::diff(const Frame& f) const
	{
		std::vector<expr_vector> out;
		std::set_difference(
				blocked_cubes.begin(), blocked_cubes.end(),
				f.blocked_cubes.begin(), f.blocked_cubes.end(),
				std::back_inserter(out), z3ext::expr_vector_less());
		return out;
	}

	std::string Frame::solver_str() const
	{
		std::string str(fmt::format("solver level {}\n", level));
		const expr_vector& asserts = consecution_solver.assertions();
		
		auto it = asserts.begin();
		for (int i = 0; i < cubes_start && it != asserts.end(); i++) it++;

		for (; it != asserts.end(); it++)
			str += fmt::format("- {}\n", (*it).to_string());

		return str;
	}

	std::string Frame::blocked_str() const
	{
		std::string str(fmt::format("blocked cubes level {}\n", level));
		for (const expr_vector& e : blocked_cubes)
			str += fmt::format("- {}\n", join_expr_vec(e, " & "));

		return str;
	}
}
