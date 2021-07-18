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

	//vector represents literals of a clause
	bool Frame::added(const expr_vector& new_clause) const 
	{ 
		if (log)
		{
			SPDLOG_LOGGER_TRACE(log, "FRAME || check if");
			SPDLOG_LOGGER_TRACE(log, "      || [ {} ]", join_expr_vec(new_clause));
			vector<expr> sorted_clause = z3ext::convert(new_clause);
			std::sort(sorted_clause.begin(), sorted_clause.end(), z3ext::expr_less());
			SPDLOG_LOGGER_TRACE(log, "      || [ {} ]", join_expr_vec(sorted_clause));
			SPDLOG_LOGGER_TRACE(log, "      || is subsumed by");
		}

		for (const expr& added_clause : added_clauses)
		{
			expr_vector added_clause_vec = z3ext::args(added_clause);
			if (log)
				SPDLOG_LOGGER_TRACE(log, "      || [ {} ] ???", join_expr_vec(added_clause_vec));
			if (z3ext::subsumes(added_clause_vec, new_clause))
			{
				if (log)
					SPDLOG_LOGGER_TRACE(log, "      || YES");
				return true; //equal or stronger clause found
			}
		}
		return false;
	}

	bool Frame::blocked(const expr_vector& cube) const 
	{ 
		return added(z3ext::negate(cube));
	}

	bool Frame::block_cube(const expr_vector& cube)
	{
		expr clause = z3::mk_or(z3ext::negate(cube));
		bool inserted = added_clauses.insert(clause).second;

		if (!inserted) //TODO possibly check some subsumption here
			return false;

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
		return added_clauses == f.added_clauses;
	}

	std::vector<expr> Frame::diff(const Frame& f) const
	{
		std::vector<expr> out;
		std::set_difference(
				added_clauses.begin(), added_clauses.end(),
				f.added_clauses.begin(), f.added_clauses.end(),
				std::back_inserter(out), z3ext::expr_less());
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
		for (const expr& e : added_clauses)
			str += fmt::format("- {}\n", e.to_string());

		return str;
	}
}