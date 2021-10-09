#include "logging.h"
#include "frames.h"
#include "frame.h"
#include "stats.h"
#include "solver.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
#include <z3++.h>

namespace pdr 
{
	Frames::Frames(bool d, z3::context& c, const PDRModel& m, Logger& l) 
		: delta(d), ctx(c), model(m), logger(l)
	{
		base_assertions.push_back(model.property.currents());
		base_assertions.push_back(model.get_transition());
		base_assertions.push_back(model.get_cardinality());

		if (delta)
			delta_solver = std::make_unique<Solver>(ctx, base_assertions);

		std::vector<z3::expr_vector> initial_assertions = {
			model.get_initial(), model.get_transition(), model.get_cardinality() 
		};
		frames.emplace_back(delta, frames.size(), ctx, logger, initial_assertions);
	}
	
	void Frames::extend()
	{
		assert(frames.size() > 0);	
		if (delta)
		{
			act.emplace_back(fmt::format("__act{}__", frames.size()));
			frames.emplace_back(delta, frames.size(), ctx, logger);
		}
		else
			frames.emplace_back(delta, frames.size(), ctx, logger, base_assertions);
	}

	void Frames::remove_state(expr_vector& cube, size_t level)
	{
		level = std::min(level, frames.size()-1);
		SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| removing cube from level [1..{}]: [{}]", 
				logger.tab(), level, join(cube));
		logger.log_indent++;

		if (delta) //a cube is only stored in the last frame it holds
			delta_remove_state(cube, level);
		else
			fat_remove_state(cube, level);
		logger.log_indent--;
	}

	void Frames::delta_remove_state(expr_vector& cube, size_t level)
	{
		for (unsigned i = 1; i <= level; i++)
		{
			//remove all blocked cubes that are equal or weaker than cube
			unsigned n_removed = frames[i]->remove_subsumed(cube); 
			logger.stats.subsumed(level, n_removed);
		}
		assert(frames[level]->is_delta());
		if (frames[level]->block_cube(cube))
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(), level);
	}

	void Frames::fat_remove_state(expr_vector& cube, size_t level)
	{
		for (unsigned i = 1; i <= level; i++)
		{
			//remove all blocked cubes that are equal or weaker than cube
			unsigned n_removed = frames[i]->remove_subsumed(cube); 
			logger.stats.subsumed(level, n_removed);

			if (frames[i]->block_cube(cube))
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(), i);
		}
	}


	//queries
	//
	bool Frames::inductive_rel_to(const z3::expr_vector& cube, size_t frame) const
	{	//query: Fi & !s & T /=> !s'
		z3::expr clause = z3::mk_or(z3ext::negate(cube)); //negate cube via demorgan
		z3::expr_vector assumptions = model.literals.p(cube);  //cube in next state
		assumptions.push_back(clause);

		if (SAT(frame, assumptions)) //there is a transition from !s to s'
			return false;

		return false;

	}

	bool Frames::transition_from_to(size_t frame, const z3::expr_vector& cube) const
	{	//query: Fi & T /=> !s'
		z3::expr_vector cube_p = model.literals.p(cube);  //cube in next state

		return SAT(frame, cube_p); //there is a transition from Fi to s'
	}

	//SAT calls
	bool Frames::SAT(size_t frame, z3::expr_vector& assumptions) const
	{
		using std::chrono::steady_clock; 

		Solver* solver = nullptr;
		if (delta && frame > 0)
		{
			assert(frames.size() == act.size());
			for (unsigned i = frame; i <= frontier(); i++)
				assumptions.push_back(act[i]);
		
			solver = delta_solver.get();
		}
		else
			solver = frames[frame]->get_solver();

		auto start = steady_clock::now();
		bool result = solver->SAT(assumptions);
		std::chrono::duration<double> diff(steady_clock::now() - start);
		logger.stats.solver_call(frontier(), diff.count());

		return result;
	}

	void Frames::discard_model(size_t frame)
	{
		if (delta && frame > 0)
			delta_solver->discard_model();
		else
			frames[frame]->get_solver()->discard_model();
	}

	//getters
	unsigned Frames::frontier() const
	{
		assert(frames.size() > 0);
		return frames.size() - 1;
	}
}
