#include "frame.h"
#include "z3-ext.h"

#include <algorithm>
#include <fmt/core.h>
#include <memory>
#include <numeric>
#include <chrono>
#include <utility>
#include <z3++.h>


namespace pdr
{
	using z3ext::join_expr_vec;

	Frame::Frame(unsigned i, const unsigned& k, context& c, Statistics& s, 
			const vector<expr_vector>& assertions, shared_ptr<spdlog::logger> l, bool d) 
		: level(i), max(k), ctx(c), stats(s), 
			base_assertions(assertions), consecution_solver(c/*, "QF_FD"*/), log(l), delta(d)
	{
		init_solver();
	}

	Frame::Frame(unsigned i, const unsigned& k, context& c, Statistics& s, 
			const vector<expr_vector>& assertions, bool d) 
		: Frame(i, k, c, s, assertions, shared_ptr<spdlog::logger>(), d) 
	{ }

	void Frame::init_solver()
	{
		consecution_solver.set("sat.cardinality.solver", true);
		consecution_solver.set("cardinality.solver", true);
		// consecution_solver.set("lookahead_simplify", true);
		for (const expr_vector& v : base_assertions)
			consecution_solver.add(v);

		cubes_start = std::accumulate(base_assertions.begin(), base_assertions.end(), 
				0, [](int agg, const expr_vector& v) { return agg + v.size(); });
	}

	void Frame::reset_solver()
	{
		consecution_solver.reset();
		
		init_solver();

		for (const expr_vector& cube : blocked_cubes)
		{
			expr clause = z3::mk_or(z3ext::negate(cube));
			consecution_solver.add(clause);
		}
	}

	void Frame::reset_frame(Statistics& s, const vector<expr_vector>& assertions)
	{
		stats = s;
		base_assertions = assertions;
		reset_solver();
	}

	//cube subsumption functions
	//
	void Frame::store_subsumed(const expr_vector& super, const expr_vector& sub)
	{
		return;
		SPDLOG_LOGGER_TRACE(log, "store for later propagation {}", z3ext::join_expr_vec(sub));
		CubeSet s = {sub};
		auto [it, inserted] = subsumed.emplace(super, std::move(s));

		if (!inserted)
		{
			it->second.insert(sub);
		}

	}

	bool Frame::blocked(const expr_vector& cube) 
	{ 
		for (const expr_vector& blocked_cube : blocked_cubes)
		{
			if (z3ext::subsumes(blocked_cube, cube))
			{
				SPDLOG_LOGGER_TRACE(log, "already blocked by {}", z3ext::join_expr_vec(blocked_cube));
				store_subsumed(blocked_cube, cube);
				return true; //equal or stronger clause found
			}
		}
		return false;	
	}
	
	unsigned Frame::remove_subsumed(const expr_vector& cube)
	{
		// return 0;
		unsigned before = blocked_cubes.size();
		// auto new_end = std::remove_if(blocked_cubes.begin(), blocked_cubes.end(),
		// 		[&cube](const expr_vector& blocked) { return z3ext::subsumes(cube, blocked); });
		for (auto it = blocked_cubes.begin(); it != blocked_cubes.end();)
		{
			if (z3ext::subsumes(cube, *it))
			{
				store_subsumed(cube, *it);
				it = blocked_cubes.erase(it);
			}
			else
				it++;
		}
		// blocked_cubes.erase(new_end, blocked_cubes.end());
		return before - blocked_cubes.size();
	}

	//interface
	//
	//cube is sorted by id()
	//block cube unless it, or a stronger version, is already blocked
	bool Frame::block_cube(const expr_vector& cube)
	{
		if (delta)
			return delta_block(cube);
		else
			return fat_block(cube);
	}

	bool Frame::fat_block(const expr_vector& cube)
	{
		if (blocked(cube)) //do not add if an equal or stronger version is already blocked
		{
			stats.blocked_ignored++;
			return false;
		}

		unsigned n_removed = remove_subsumed(cube); //remove all blocked cubes that are weaker than cube
		stats.subsumed(level, n_removed);

		// blocked_cubes.push_back(cube);
		bool inserted = blocked_cubes.insert(cube).second; assert(inserted);

		expr clause = z3::mk_or(z3ext::negate(cube));
		consecution_solver.add(clause);
		return true;
	}

	expr Frame::activation_lit(unsigned level) const
	{
		std::string name = fmt::format("__act{}__", level);
		return ctx.bool_const(name.c_str());
	}

	bool Frame::delta_block(expr_vector cube)
	{
		cube.push_back(activation_lit(level));
		z3ext::sort(cube);
		//add clause with !act
		bool inserted = blocked_cubes.insert(cube).second; assert(inserted);

		expr clause = z3::mk_or(z3ext::negate(cube));
		consecution_solver.add(clause);
		return true;
	}

	bool Frame::SAT(const expr& current, expr_vector next)
	{
		next.push_back(current);
		return SAT(next);
	}

	bool Frame::SAT(const expr_vector& assumptions) 
	{ 
		using std::chrono::steady_clock; 
		auto start = steady_clock::now();
		bool result;

		if (delta)	
			result = delta_SAT(assumptions);
		else
			result = fat_SAT(assumptions);

		std::chrono::duration<double> diff(steady_clock::now() - start);
		stats.solver_call(level, diff.count());
		return result;
	}

	bool Frame::fat_SAT(const expr_vector& assumptions)
	{
		// if (log)
		// {
		// 	SPDLOG_LOGGER_TRACE(log, "SAT | assertions:\n {}", solver_str());
		// 	SPDLOG_LOGGER_TRACE(log, "SAT | assumptions:\n {}", z3ext::join_expr_vec(assumptions, false));
		// }
		if (consecution_solver.check(assumptions) == z3::sat)
		{
			if(!model_used)
				std::cerr << "PDR::WARNING: last SAT model unused and discarded" << std::endl;
			model_used = false;
			return true;
		}

		core_available = true;
		return false;
	}

	bool Frame::delta_SAT(expr_vector assumptions) //fmcad: return lowest used act
	{
		for (unsigned i = level; i <= max; i++)
			assumptions.push_back(activation_lit(i));
		
		return fat_SAT(assumptions);
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

	expr_vector Frame::unsat_core()
	{
		assert(core_available);
		expr_vector core = consecution_solver.unsat_core();
		core_available = false;
		return core;
	}

	// assumes vectors in 'blocked_cubes' are sorted
	bool Frame::equals(const Frame& f) const
	{ 
		auto eq_return = [](bool rv) {
			// std::cout << fmt::format("F{} and F{}", this->level, f.level) 
			// 		  << (rv ? "equal" : "not equal") << std::endl;
			return rv;
		};

		if (this->blocked_cubes.size() != f.blocked_cubes.size())
			return eq_return(false);
		
		auto l_cube = this->blocked_cubes.begin(); auto r_cube = f.blocked_cubes.begin();
		auto l_end = this->blocked_cubes.end(); auto r_end = f.blocked_cubes.end();
		for (; l_cube != l_end && r_cube != r_end; l_cube++, r_cube++)
		{
			// if l_cubes* != r_cubes* -> return false
			if (l_cube->size() != r_cube->size())
				return eq_return(false);

			auto l_lit = l_cube->begin(); auto r_lit = r_cube->begin();
			for (; l_lit != l_cube->end() && r_lit != r_cube->end(); l_lit++, r_lit++)
				if ((*l_lit).id() != (*r_lit).id())
					return eq_return(false);
		}
		return eq_return(true);
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

	const CubeSet& Frame::get_blocked_cubes() const { return blocked_cubes; }
	bool Frame::empty() const { return blocked_cubes.size() == 0; }

	std::string Frame::solver_str() const
	{
		std::string str(fmt::format("solver level {}\n", level));
		const expr_vector asserts = consecution_solver.assertions();
		
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
