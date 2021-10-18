#include "frames.h"
#include "frame.h"
#include "logging.h"
#include "stats.h"
#include "solver.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <vector>
#include <z3++.h>

namespace pdr 
{
	Frames::Frames(bool d, z3::context& c, const PDRModel& m, Logger& l) 
		: delta(d), ctx(c), model(m), logger(l), init_solver(ctx)
	{
		init_solver.add(model.get_initial());
		base_assertions.push_back(model.property.currents());
		base_assertions.push_back(model.get_transition());
		base_assertions.push_back(model.get_cardinality());

		if (delta)
			delta_solver = std::make_unique<Solver>(ctx, base_assertions);

		std::vector<z3::expr_vector> initial_assertions = 
			{ model.get_initial(), model.get_transition(), model.get_cardinality()	};
		act.emplace_back(ctx.bool_const("__actI__")); //unused
		frames.push_back(std::make_unique<Frame>(frames.size(), new Solver(ctx, initial_assertions), logger));
	}

	//frame interface
	//
	
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

	void Frames::reset_frames(Statistics& s, std::vector<z3::expr_vector> assertions)
	{
		for (auto& f : frames)
			f->reset_frame(s, assertions);
	}

	bool Frames::remove_state(const z3::expr_vector& cube, size_t level)
	{
		level = std::min(level, frames.size()-1);
		SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| removing cube from level [1..{}]: [{}]", 
				logger.tab(), level, str::extensions::join(cube));
		logger.log_indent++;

		bool result;
		if (delta) //a cube is only stored in the last frame it holds
			result = delta_remove_state(cube, level);
		else
			result = fat_remove_state(cube, level);
		logger.log_indent--;
		return result;
	}

	bool Frames::delta_remove_state(const z3::expr_vector& cube, size_t level)
	{
		for (unsigned i = 1; i <= level; i++)
		{
			//remove all blocked cubes that are equal or weaker than cube
			unsigned n_removed = frames.at(i)->remove_subsumed(cube); 
			logger.stats.subsumed(level, n_removed);
		}

		assert(level > 0);
		if (frames.at(level)->block(cube)) //only blocks cube in frame, not in solver
		{
			delta_solver->block(cube, act.at(level));
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(), level);
			return true;
		}
		return false;
	}

	bool Frames::fat_remove_state(const z3::expr_vector& cube, size_t level)
	{
		assert(level > 0);
		for (unsigned i = 1; i <= level; i++)
		{
			//remove all blocked cubes that are equal or weaker than cube
			unsigned n_removed = frames.at(i)->remove_subsumed(cube); 
			logger.stats.subsumed(level, n_removed);

			if (frames.at(i)->block(cube))
			{
				frames.at(i)->block_in_solver(cube);
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(), i);
			}
			else
				return false;
		}
		return true;
	}

	bool Frames::propagate(unsigned level, bool repeat)
	{
		assert(level == frontier()-1); // k == |F|-1
		std::cout << "propagate level " << level << endl;

		if (!delta) frames.at(1)->reset_solver();
		for (unsigned i = 1; i <= level; i++)
		{
			if (delta)
				push_forward_delta(i, repeat);
			else
				if (push_forward_fat(i, repeat))
					return true;
		}

		if (delta)
		{
			for (unsigned i = 1; i < frames.size(); i++)
				if (frames.at(i)->empty())
				{
					std::cout << format("F[{}] \\ F[{}] == 0", i, i+1) << std::endl;
					return true;
				}
			delta_solver->reset();
		}

		return false;
	}

	void Frames::push_forward_delta(unsigned level, bool repeat)
	{
		for (const z3::expr_vector& cube : frames.at(level)->get_blocked())
		{
			if (!transition_from_to(level, cube))
			{
				if (remove_state(cube, level+1))
					if (repeat)
						std::cout << "new blocked in repeat" << endl;
			}
			else
				delta_solver->discard_model();
		}
	}

	bool Frames::push_forward_fat(unsigned level, bool repeat)
	{
		std::vector<z3::expr_vector> diff = frames.at(level)->diff(*frames.at(level+1));
		for (const z3::expr_vector& cube : diff)
		{
			if (!transition_from_to(level, cube))
			{
				if (remove_state(cube, level+1))
					if (repeat)
						std::cout << "new blocked in repeat" << endl;
			}
			else
				solver(level)->discard_model();
		}

		if (diff.size() == 0 || frames.at(level)->equals(*frames.at(level+1)))
		{
			std::cout << format("F[{}] \\ F[{}] == 0", level, level+1) << std::endl;
			return true;
		}

		return false;
	}


	//
	//end frame interface

	//queries
	//
	bool Frames::neg_inductive_rel_to(const std::vector<z3::expr>& cube, size_t frame) const
	{
		z3::expr_vector ev(ctx);
		for (const z3::expr& e : cube)
			ev.push_back(e);
		return neg_inductive_rel_to(ev, frame);
	}

	bool Frames::neg_inductive_rel_to(const z3::expr_vector& cube, size_t frame) const
	{	//query: Fi & !s & T /=> !s'
		z3::expr clause = z3::mk_or(z3ext::negate(cube)); //negate cube via demorgan
		z3::expr_vector assumptions = model.literals.p(cube);  //cube in next state
		assumptions.push_back(clause);

		if (SAT(frame, assumptions)) //there is a transition from !s to s'
			return false;

		return false;

	}

	//if primed: cube is already in next state, else first convert it
	bool Frames::transition_from_to(size_t frame, const z3::expr_vector& cube, bool primed) const
	{
		if (!primed)//cube is in current state, bring to next
			return SAT(frame, model.literals.p(cube)); //there is a transition from Fi to s'

		return SAT(frame, cube); //there is a transition from Fi to s'
	}

	//
	//end queries

	//SAT interface
	//
	bool Frames::SAT(size_t frame, const z3::expr_vector& assumptions) const
	{
		z3::expr_vector ass_copy = assumptions; //since we modify assumptions in SAT
		return SAT(frame, ass_copy);
	}

	bool Frames::SAT(size_t frame, z3::expr_vector& assumptions) const
	{
		using std::chrono::steady_clock; 

		Solver* solver = nullptr;
		if (delta && frame > 0)
		{
			assert(frames.size() == act.size());
			for (unsigned i = frame; i <= frontier(); i++)
				assumptions.push_back(act.at(i));
		
			solver = delta_solver.get();
		}
		else
			solver = frames.at(frame)->get_solver();

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
			frames.at(frame)->get_solver()->discard_model();
	}

	void Frames::reset_solver(size_t frame)
	{
		if (delta && frame > 0)
		{
			delta_solver->reset();

			for (const z3::expr_vector& cube : frames.at(frame)->get_blocked())
			{
				if (delta)
					delta_solver->block(cube, act.at(frame));
				else
					solver(frame)->block(cube);
			}
		}
		else
			frames.at(frame)->reset_solver();
	}

	//
	//end SAT interface

	//getters
	//
	unsigned Frames::frontier() const
	{
		assert(frames.size() > 0);
		return frames.size() - 1;
	}

	Solver* Frames::solver(size_t frame)
	{
		if (delta && frame > 0)
			return delta_solver.get();
		return frames.at(frame)->get_solver();
	}

	const Frame& Frames::operator[](size_t i)
	{
		return *frames.at(i);
	}

	//
	//end getters

	void Frames::log_solvers() const
	{
		for (const std::unique_ptr<Frame>& f : frames)
		{
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}", (*f).get_solver()->as_str());
		}
	}

	std::string Frames::blocked_str() const
	{
		std::string str;
		for (auto& f : frames)
		{
			str += f->blocked_str();
			str += '\n';
		}
		return str;
	}
	std::string Frames::solvers_str() const
	{
		std::string str;
		if (delta)
			str += delta_solver->as_str();
		else
		{
			for (auto& f : frames)
			{
				str += f->get_solver()->as_str();
				str += '\n';
			}
		}
		return str;
	}

}
