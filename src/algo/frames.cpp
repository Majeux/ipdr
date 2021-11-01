#include "frames.h"
#include "_logging.h"
#include "frame.h"
#include "solver.h"
#include "stats.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
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

        std::vector<z3::expr_vector> initial_assertions = {
            model.get_initial(), model.get_transition(),
            model.get_cardinality()};
        act.push_back(ctx.bool_const("__actI__")); // unused
        frames.push_back(std::make_unique<Frame>(frames.size(), ctx,
                                                 initial_assertions, logger));
    }

    // frame interface
    //

    void Frames::extend()
    {
        assert(frames.size() > 0);
        if (delta)
        {
            std::string acti = fmt::format("__act{}__", frames.size());
            act.push_back(ctx.bool_const(acti.c_str()));
            frames.push_back(std::make_unique<Frame>(frames.size(), logger));
        }
        else
            frames.push_back(std::make_unique<Frame>(frames.size(), ctx,
                                                     base_assertions, logger));
    }

	// prepare frames for a new run:
	// - possibly define new statistics
	// - provide new {property, transition, cardinality} to the solvers
	// then clean all solvers from old assertions
	void Frames::reset_frames(Statistics& s,
                              const std::vector<z3::expr_vector>& assertions)
	{
		if (delta)
			delta_solver->base_assertions = assertions;
		
		for (size_t i = 1; i < frames.size(); i++)
		{
			frames[i]->set_stats(s);
			if (!delta)
				solver(i)->base_assertions = assertions;
		}

		clean_solvers();
	}

	//reset solvers and repopulate with current blocked cubes
	void Frames::clean_solvers()
    {
		for (size_t i = 1; i < frames.size(); i++)
		{
			auto& f = frames[i];

			if (delta)
			{
				if (i == 1)
					delta_solver->reset();
				for (const z3::expr_vector& cube : f->get_blocked())
					delta_solver->block(cube, act.at(i));
			}
			else
			{
				solver(i)->reset(f->get_blocked());
			}
		}
    }

    bool Frames::remove_state(const z3::expr_vector& cube, size_t level)
    {
        level = std::min(level, frames.size() - 1);
        SPDLOG_LOGGER_TRACE(logger.spd_logger,
                            "{}| removing cube from level [1..{}]: [{}]",
                            logger.tab(), level, str::extend::join(cube));
        logger.indent++;

        bool result;
        if (delta) // a cube is only stored in the last frame it holds
            result = delta_remove_state(cube, level);
        else
            result = fat_remove_state(cube, level);
        logger.indent--;
        return result;
    }

    bool Frames::delta_remove_state(const z3::expr_vector& cube, size_t level)
    {
        for (unsigned i = 1; i <= level; i++)
        {
            // remove all blocked cubes that are equal or weaker than cube
            unsigned n_removed = frames.at(i)->remove_subsumed(cube);
            logger.stats.subsumed_cubes.add(level, n_removed);
        }

        assert(level > 0);
        if (frames.at(level)->block(cube))
        {
            delta_solver->block(cube, act.at(level));
            SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}",
                                logger.tab(), level);
            return true;
        }
        return false;
    }

    bool Frames::fat_remove_state(const z3::expr_vector& cube, size_t level)
    {
        assert(level > 0);
        for (unsigned i = 1; i <= level; i++)
        {
            // remove all blocked cubes that are equal or weaker than cube
            unsigned n_removed = frames.at(i)->remove_subsumed(cube);
            logger.stats.subsumed_cubes.add(level, n_removed);

            if (frames.at(i)->block(cube))
            {
                frames.at(i)->block_in_solver(cube);
                SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}",
                                    logger.tab(), i);
            }
            else
                return false;
        }
        return true;
    }

    bool Frames::propagate(unsigned level, bool repeat)
    {
        assert(level == frontier() - 1); // k == |F|-1
        std::cout << "propagate level " << level << endl;
		SPDLOG_LOGGER_TRACE(logger.spd_logger,
							"{}| propagate frame {} to {}", logger.tab(), 1,
							level);
		logger.indent++;

        for (unsigned i = 1; i <= level; i++)
        {
            if (delta)
                push_forward_delta(i, repeat);
            else if (push_forward_fat(i, repeat))
                return true;
        }

        if (delta)
            for (unsigned i = 1; i <= level; i++)
			{
                if (frames.at(i)->empty())
                {
                    std::cout << format("F[{}] \\ F[{}] == 0", i, i + 1)
                              << std::endl;
                    return true;
                }
			}

		clean_solvers();
		logger.indent--;

        return false;
    }

    void Frames::push_forward_delta(unsigned level, bool repeat)
    {
        CubeSet blocked = frames.at(level)->get_blocked();
        for (const z3::expr_vector& cube : blocked)
        {
            if (!trans_from_to(level, cube))
            {
                if (remove_state(cube, level + 1))
                    if (repeat)
                        std::cout << "new blocked in repeat" << endl;
            }
        }
    }

    bool Frames::push_forward_fat(unsigned level, bool repeat)
    {
        std::vector<z3::expr_vector> diff =
            frames.at(level)->diff(*frames.at(level + 1));
        for (const z3::expr_vector& cube : diff)
        {
            if (!trans_from_to(level, cube))
            {
                if (remove_state(cube, level + 1))
                    if (repeat)
                        std::cout << "new blocked in repeat" << endl;
            }
        }

        if (diff.size() == 0 || frames.at(level)->equals(*frames.at(level + 1)))
        {
            std::cout << format("F[{}] \\ F[{}] == 0", level, level + 1)
                      << std::endl;
            return true;
        }

        return false;
    }
    //
    // end frame interface

    // queries
    //
    bool Frames::inductive(const std::vector<z3::expr>& cube,
                           size_t frame) const
    {
        return inductive(z3ext::convert(cube), frame);
    }

	// verifies if !cube is inductive relative to F_[frame]
	// query: Fi & !s & T /=> !s'
    bool Frames::inductive(const z3::expr_vector& cube, size_t frame) const
    { 
        SPDLOG_LOGGER_TRACE(logger.spd_logger,
                            "{}| check relative inductiveness, frame {}",
                            logger.tab(), frame);
        z3::expr clause =
            z3::mk_or(z3ext::negate(cube)); // negate cube via demorgan
        z3::expr_vector assumptions =
            model.literals.p(cube); // cube in next state
        assumptions.push_back(clause);

        if (SAT(frame, std::move(assumptions))) 
		{   // there is a transition from !s to s'
            return false;
		}

        return true;
    }

    Witness Frames::counter_to_inductiveness(const std::vector<z3::expr>& cube,
                                             size_t frame) const
    {
        SPDLOG_LOGGER_TRACE(logger.spd_logger,
                            "{}| counter to relative inductiveness, frame {}",
                            logger.tab(), frame);
        if (!inductive(cube, frame))
            return std::make_unique<z3::model>(get_model(frame));

        return std::unique_ptr<z3::model>();
    }

    Witness Frames::counter_to_inductiveness(const z3::expr_vector& cube,
                                             size_t frame) const
    {
        if (!inductive(cube, frame))
            return std::make_unique<z3::model>(get_model(frame));

        return std::unique_ptr<z3::model>();
    }

    // if primed: cube is already in next state, else first convert it
    bool Frames::trans_from_to(size_t frame, const z3::expr_vector& cube,
                               bool primed) const
    {
        SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| transition check, frame {}",
                            logger.tab(), frame);
        if (!primed) // cube is in current, bring to next
            return SAT(frame, model.literals.p(cube));

        return SAT(frame, cube); // there is a transition from Fi to s'
    }

    Witness Frames::get_trans_from_to(size_t frame, const z3::expr_vector& cube,
                                      bool primed) const
    {
        SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| transition query, frame {}",
                            logger.tab(), frame);
        if (!primed) // cube is in current, bring to next
            return SAT_model(frame, model.literals.p(cube));

        return SAT_model(frame, cube); // there is a transition from Fi to s'
    }

    //
    // end queries

    // SAT interface
    //
    bool Frames::SAT(size_t frame, const z3::expr_vector& assumptions) const
    {
        return SAT(frame, z3ext::copy(assumptions));
    }

	// the expr_vector assumptions are modified by acts, 
	// and should be considered usable afterwards
    bool Frames::SAT(size_t frame, z3::expr_vector&& assumptions) const
    {
        using std::chrono::steady_clock;

        Solver* solver = nullptr;
        if (delta && frame > 0)
        {
            SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| Delta check",
                                logger.tab());

            assert(frames.size() == act.size());
            for (unsigned i = frame; i <= frontier(); i++)
                assumptions.push_back(act.at(i));

            solver = delta_solver.get();
        }
        else
        {
            SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| Fat check",
                                logger.tab());
            solver = frames.at(frame)->get_solver();
        }

        auto start = steady_clock::now();

        logger.indent++;
        SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| assumps: [ {} ]",
                            logger.tab(),
                            z3ext::join_expr_vec(assumptions, false));

        logger.indent--;
        bool result = solver->SAT(assumptions);
        std::chrono::duration<double> diff(steady_clock::now() - start);
        logger.stats.solver_calls.add(frontier(), diff.count());

        return result;
    }

    Witness Frames::SAT_model(size_t frame,
                              const z3::expr_vector& assumptions) const
    {
        if (SAT(frame, assumptions))
            return std::make_unique<z3::model>(get_model(frame));
        return std::unique_ptr<z3::model>();
    }

    Witness Frames::SAT_model(size_t frame, z3::expr_vector&& assumptions) const
    {
        if (SAT(frame, assumptions))
            return std::make_unique<z3::model>(get_model(frame));
        return std::unique_ptr<z3::model>();
    }

    const z3::model Frames::get_model(size_t frame) const
    {
        if (delta && frame > 0)
            return delta_solver->get_model();
        else
            return frames.at(frame)->get_solver()->get_model();
    }

    //
    // end SAT interface

    // getters
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

    const Frame& Frames::operator[](size_t i) { return *frames.at(i); }

    //
    // end getters

    void Frames::log_solvers() const
    {
		SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
        if (delta)
            SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}",
                                delta_solver->as_str());
        else
            for (const std::unique_ptr<Frame>& f : frames)
            {
                SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}",
                                    (*f).get_solver()->as_str());
            }
		SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
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

} // namespace pdr
