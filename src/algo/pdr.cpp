#include "pdr.h"
#include "z3-ext.h"
#include "string-ext.h"

#include <functional>
#include <iterator>
#include <cassert>
#include <algorithm>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <fmt/format.h>
#include <fstream>
#include <tuple>
#include <z3++.h>

namespace pdr
{
	PDR::PDR(PDRModel& m, bool d = false) 
		: ctx(m.ctx), model(m), delta(d), logger(model.name), frames(delta, ctx, m, logger)
	{}

	void PDR::reset()
	{
		logger.log_indent = 0;
		bad = std::shared_ptr<State>();
		stats = Statistics();
		frames_string = "None";
		solvers_string = "None";
	}

	void PDR::print_model(const z3::model& m)
	{
		std::cout << "model consts \{" << std::endl;
		for (unsigned i = 0; i < m.num_consts(); i++)
		{
			std::cout << "\t" << m.get_const_interp( m.get_const_decl(i) );
		}
		std::cout << "}" << endl;
	}

	bool PDR::run(bool dynamic)
	{
		dynamic_cardinality = dynamic;
		reset();
		timer.reset();

		assert(k == frames.frontier());

		bool failed = false;
		std::cout << endl << "PDR start:" << endl;
		SPDLOG_LOGGER_INFO(logger.spd_logger, "");
		SPDLOG_LOGGER_INFO(logger.spd_logger, "NEW RUN\n");
		SPDLOG_LOGGER_INFO(logger.spd_logger, "PDR start");
		
		if (!dynamic || k == 0)
		{
			SPDLOG_LOGGER_INFO(logger.spd_logger, "Start initiation");
			logger.log_indent++;
			failed = !init();
			logger.log_indent--;
		}

		if (failed)
		{
			std::cout << "Failed initiation" << endl;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "Failed initiation");
			return finish(false);
		}
		std::cout << "Survived initiation" << endl;
		SPDLOG_LOGGER_INFO(logger.spd_logger, "Survived initiation");

		SPDLOG_LOGGER_INFO(logger.spd_logger, "Start iteration");
		logger.log_indent++;
		failed = !iterate();
		logger.log_indent--;

		if(failed) 
		{
			std::cout << "Failed iteration" << endl;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "Failed iteration");
			return finish(false);
		}

		std::cout << "Property verified" << endl;
		SPDLOG_LOGGER_INFO(logger.spd_logger, "Property verified");
		return finish(true);
	}

	bool PDR::finish(bool result) 
	{
		double final_time = timer.elapsed().count();
		std::cout << format("Total elapsed time {}", final_time) << endl;
		SPDLOG_LOGGER_INFO(logger.spd_logger, "Total elapsed time {}", final_time);
		stats.elapsed = final_time;

		store_frame_strings();
		if (dynamic_cardinality)
			store_frames();

		return result;
	}

	// returns true if the model survives initiation
	bool PDR::init() 
	{
		assert(frames.frontier() == 0);

		SPDLOG_LOGGER_TRACE(logger.spd_logger, "Start initiation");
		z3::expr_vector notP = model.not_property.currents();
		if (frames.init_solver.check(notP))
		{
			std::cout << "I =/> P" << std::endl;
			z3::model counter = frames[0].get_solver()->get_model();
			print_model(counter);
			//TODO TRACE	
			bad = std::make_shared<State>(model.get_initial());
			return false;
		}

		z3::expr_vector notP_next = model.not_property.nexts();
		if ( frames.SAT(0, notP_next) )
		{	//there is a transitions from I to !P
			std::cout << "I & T =/> P'" << std::endl;
			expr_vector bad_cube = frames[0].get_solver()->sat_cube(
					[this](const expr& e) { return model.literals.atom_is_current(e); });
			bad = std::make_shared<State>(bad_cube);

			return false;
		}

		frames.extend();
		k = 1;
		assert(k == frames.frontier());

		return true;
	}

	bool PDR::iterate()
	{
		std::cout << SEP3 << endl;
		std::cout << "Start iteration" << endl;

		// I => P and I & T ⇒ P' (from init)
		while (true) //iterate over k, if dynamic this continues from last k
		{
			std::cout << "iterate frame "<< k << endl;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "");
			SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| frame {}", logger.tab(), k);
			assert(k == frames.frontier());

			while (true) //exhaust all transitions to !P
			{
				if (frames.transition_from_to(k, model.not_property.currents())) //TODO naar nexts impliciet
				{
						// F_i & T /=> F_i+1' (= P')
					// strengthen F_i
					SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| cti found at frame {}", logger.tab(), k);
					std::cout << "new cti" << endl;
					logger.log_indent++;
					
					expr_vector cti_current = frames[k].get_solver()->sat_cube(
							[this](const expr& e) { return model.literals.atom_is_current(e); });

					SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(), join(cti_current));	
					logger.log_indent--;

						// s is not in F_k-1 (or it would have been found previously)
						// F_k-2 & T & !s => !s'
						// only need to to search k-1 ... k
					expr_vector core(ctx);
					int n = highest_inductive_frame(cti_current, (int)k - 1, (int)k, core);
						// int n = highest_inductive_frame(cti_current, (int)k - 1, (int)k);
					assert(n >= 0);

						// F_n & T & !s => !s
						// F_n & T => F_n+1
					expr_vector smaller_cti = generalize(core, n);
					remove_state(smaller_cti, n + 1);

					SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| block", logger.tab());
					logger.log_indent++;
					
					if (not block(cti_current, n+1, k)) 
						return false;
					logger.log_indent--;
					SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP2);
					std::cout << endl;
				}
				else // no more counter examples
					break;
			}

			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| propagate frame {} to {}", logger.tab(), 1, k);
			logger.log_indent++;

			frames.extend();
			bool  done = propagate(k);
			k++;
			
			logger.log_indent--;
			std::cout << "###############" << endl;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3); 
			for (const unique_ptr<Frame>& f : frames)
			{
				// if (k == 7) 
				// {
				// 	std::fstream fs;
				// 	fs.open ("test/dyn10-6-7.txt", std::fstream::in | std::fstream::app);
				// 	fs << (*f).solver_str() << endl;
				// 	fs.close();

				// }
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}", (*f).solver_str());
			}
			SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3);

			if (done)
				return true;

		}
	}

	bool PDR::block(expr_vector& cti, unsigned n, unsigned level)
	{
		unsigned period = 0;
		std::set<Obligation, std::less<Obligation>> obligations; 
		if ((n + 1) <= level)
			obligations.emplace(n+1, std::move(cti), 0);

		// forall (n, state) in obligations: !state->cube is inductive relative to F[i-1]
		while (obligations.size() > 0)
		{
			sub_timer.reset();
			double elapsed; string branch;

			auto [n, state, depth] = *(obligations.begin());

			assert(n <= level);
			SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP);
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| obligations pending: {}", logger.tab(), obligations.size());
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| top obligation", logger.tab());
			logger.log_indent++;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| {}, [{}]", logger.tab(), n, join(state->cube));
			logger.log_indent--;

			expr state_clause = z3::mk_or(z3ext::negate(state->cube));

			if ( frames[n]->SAT(state_clause, model.literals.p(state->cube)) )
			{	//predecessor found
				expr_vector pred_cube = frames[n]->sat_cube(
							[this](const expr& e) { return model.literals.atom_is_current(e); });
				shared_ptr<State> pred = std::make_shared<State>(pred_cube, state);

				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| predecessor found", logger.tab());
				logger.log_indent++;
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(), join(pred->cube));
				logger.log_indent--;

				expr_vector core(ctx);
				// state is inductive relative to F[n-2]
				// else there would be an F[k-1] state t preceding state, making state an F[k-1] state
				int m = highest_inductive_frame(pred->cube, n-1, level, core);
				// int m = highest_inductive_frame(pred->cube, n-1, level);
				// m in [n-1, level]
				if (m >= 0)
				{
					expr_vector smaller_pred = generalize(core, m);
					// expr_vector smaller_pred = generalize(pred->cube, m);
					remove_state(smaller_pred, m + 1);

					if (static_cast<unsigned>(m+1) <= level)
					{
						SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| push predecessor to level {}: [{}]", logger.tab(), m+1, join(pred->cube));
						obligations.emplace(m+1, pred, depth+1);
					}
				}
				else //intersects with I
				{ 
					bad = pred;
					return false;
				}
				elapsed = sub_timer.elapsed().count();
				branch = "(pred)  ";
			}
			else 
			{	//finish state
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| finishing state", logger.tab());
				logger.log_indent++;
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(), join(state->cube));
				logger.log_indent--;

				expr_vector core(ctx);
				// !state is now inductive relative to n
				int m = highest_inductive_frame(state->cube, n + 1, level, core);
				// int m = highest_inductive_frame(state->cube, n + 1, level);
				// m in [n-1, level]
				assert(static_cast<unsigned>(m+1) > n);

				if (m >= 0)
				{
					expr_vector smaller_state = generalize(core, m);
					// expr_vector smaller_state = generalize(state->cube, m);
					remove_state(smaller_state, m + 1);
					obligations.erase(obligations.begin()); //problem with & structured binding??

					if (static_cast<unsigned>(m+1) <= level)
					{
						SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| push state to higher to level {}: [{}]", logger.tab(), m+1, join(state->cube));
						obligations.emplace(m+1, state, depth);
					}
				}
				else
				{
					bad = state;
					return false;
				}
				elapsed = sub_timer.elapsed().count();
				branch = "(finish)";
			}
			stats.obligation_done(level, elapsed);
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "Obligation {} elapsed {}", branch, elapsed);
			std::cout << format("Obligation {} elapsed {}", branch, elapsed) << endl;
			elapsed = -1.0;

			if (period >= 100)
			{
				period = 0;
				std::cout << "Stats written" << endl;
				SPDLOG_LOGGER_DEBUG(logger.spd_logger, stats.to_string());
				logger.spd_logger->flush();
			}
			else period++;
		}
		return true;
	}

	void PDR::remove_state(expr_vector& cube, unsigned level)
	{
		level = std::min(static_cast<size_t>(level), frames.size()-1);
		SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| removing cube from level [1..{}]: [{}]", logger.tab(), level, join(cube));
		logger.log_indent++;

		if (delta) //a cube is only stored in the last frame it holds
		{
			for (unsigned i = 1; i <= level; i++)
			{
				if (frames[i]->blocked(cube)) //do not add if an equal or stronger version is already blocked
				{
					stats.blocked_ignored++;
					break;
				}
				//remove all blocked cubes that are weaker than cube
				unsigned n_removed = frames[i]->remove_subsumed(cube); 
				stats.subsumed(level, n_removed);
				
				if (frames[level]->block_cube(cube))
					SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(), level);
			}
		}
		else
		{
			for (unsigned i = 1; i <= level; i++)
				if (frames[i]->block_cube(cube))
					SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| blocked in {}", logger.tab(), i);
		}
		logger.log_indent--;
	}

	bool PDR::propagate(unsigned level, bool repeat)
	{
		assert(level == frames.size()-2); // k == |F|-1
		sub_timer.reset();
		std::cout << "propagate level " << level << endl;

		bool invariant;
		if (delta)
			invariant = delta_propagate(level, repeat);
		else
			invariant = fat_propagate(level, repeat);

		SPDLOG_LOGGER_TRACE(logger.spd_logger, "Propagation elapsed {}", sub_timer);
		std::cout << format("Propagation elapsed {}", sub_timer) << endl;
		return invariant;
	}

	bool PDR::fat_propagate(unsigned level, bool repeat)
	{
		frames[1]->reset_solver();
		//extracts arguments of e as an expr_vector
		for (unsigned i = 1; i <= level; i++)
		{   //TODO: check in of exclusief
			vector<expr_vector> diff = frames[i]->diff(*frames[i+1]);

			for (const expr_vector& cube : diff)
			{
				expr_vector cube_next = model.literals.p(cube);

				if (frames[i]->UNSAT(cube_next))
				{
					if (frames[i+1]->block_cube(cube))
						if (repeat)
							std::cout << "new blocked in repeat" << endl;
						// trace.AddedClauses++;
					// else
					// {
						// IC3Trace.LogLine("Clause is subsumed in frame " + i, "Propagate" );
						// trace.Subsumed++;
					// }
				}
				else 
					frames[i]->discard_model();
			}
			frames[i+1]->reset_solver();

			if (diff.size() == 0 || frames[i]->equals(*frames[i + 1]))
			{
				std::cout << format("F[{}] \\ F[{}] == 0", i, i+1) << std::endl;
				return true;
			}
		}
		return false;
	}

	bool PDR::delta_propagate(unsigned level, bool repeat)
	{
		frames[1]->reset_solver();
		for (unsigned i = 1; i <= level; i++)
		{
			for (const expr_vector& cube : frames[i]->get_blocked_cubes())
			{
				expr_vector cube_next = model.literals.p(cube);
				if (frames[i]->UNSAT(cube_next))
				{
					if (frames[i+1]->block_cube(cube))
						if (repeat)
							std::cout << "new blocked in repeat" << endl;
				}
				else
					frames[i]->discard_model();
			}
			frames[i+1]->reset_solver();
			
			if (frames[i]->empty())
			{
				std::cout << format("F[{}] \\ F[{}] == 0", i, i+1) << std::endl;
				return true;
			}
		}
		return false;
	}

	void PDR::store_frame_strings() 
	{
		std::stringstream ss;

		ss << "Frames" << endl;
		for (const unique_ptr<Frame>& f : frames)
			ss << (*f).blocked_str() << endl;

		frames_string = ss.str();

		ss = std::stringstream();

		ss << "Solvers" << endl;
		for (const unique_ptr<Frame>& f : frames)
				ss << (*f).solver_str() << endl;

		solvers_string = ss.str();
	}

	void PDR::show_results(std::ostream& out) const
	{
		out << format("Results pebbling strategy with {} pebbles for {}", model.get_max_pebbles(), model.name) << endl;
		out << SEP2 << endl;

		if (bad)
		{
			out << "Bad state reached:" << endl;
			out << format("[ {} ]", join_expr_vec(model.not_property.currents(), " & ")) << endl << endl;

			out << "Reached from:" << endl;
			show_trace(out);
			out << SEP2 << endl;
		}
		else 
		{
			out << fmt::format("No strategy for {} pebbles", model.get_max_pebbles()) << endl << endl;
		}

		out << frames_string << endl;
		out << SEP << endl;
		out << solvers_string << endl;
	}

	void PDR::show_trace(std::ostream& out) const
	{
		std::vector<std::tuple<unsigned, string, unsigned>> steps;

		shared_ptr<State> current = bad;
		auto count_pebbled = [](const expr_vector& vec) 
		{
			unsigned count = 0;
			for (const expr& e : vec)
				if(!e.is_not())
					count++;

			return count; 
		};

		unsigned i = 0;
		while (current)
		{
			i++;
			steps.emplace_back(
					i, 
					join_expr_vec(current->cube), 
					count_pebbled(current->cube)
				);
			current = current->prev;
		}
		unsigned i_padding = i/10 + 1;

		out << format("{:>{}} |\t [ {} ]", 'I', i_padding, join_expr_vec(model.get_initial())) << endl;

		for (const auto &[num, vec, count] : steps)
			out << format("{:>{}} |\t [ {} ] No. pebbled = {}", num, i_padding, vec, count) << endl; 

		out << format("{:>{}} |\t [ {} ]", 'F', i_padding, join_expr_vec(model.not_property.currents())) << endl;
	}
}
