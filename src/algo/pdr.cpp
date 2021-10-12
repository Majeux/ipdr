#include "pdr.h"
#include "stats.h"
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
		logger.stats = Statistics();
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
			z3::model counter = frames.solver(0)->get_model();
			print_model(counter);
			//TODO TRACE	
			bad = std::make_shared<State>(model.get_initial());
			return false;
		}

		z3::expr_vector notP_next = model.not_property.nexts();
		if ( frames.SAT(0, notP_next) )
		{	//there is a transitions from I to !P
			std::cout << "I & T =/> P'" << std::endl;
			expr_vector bad_cube = frames.solver(0)->witness(
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
				if (frames.transition_from_to(k, model.not_property.nexts(), true))
				{
						// F_i & T /=> F_i+1' (= P')
					// strengthen F_i
					SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| cti found at frame {}", logger.tab(), k);
					std::cout << "new cti" << endl;
					logger.log_indent++;
					
					expr_vector cti_current = frames.solver(k)->witness(
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
					frames.remove_state(smaller_cti, n + 1);

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
			sub_timer.reset();
			bool done = frames.propagate(k);
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "Propagation elapsed {}", sub_timer);
			std::cout << format("Propagation elapsed {}", sub_timer) << endl;
			k++;
			
			logger.log_indent--;
			std::cout << "###############" << endl;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, SEP3); 
			frames.log_solvers();
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
			SPDLOG_LOGGER_TRACE(logger.spd_logger, 
					"{}| obligations pending: {}", logger.tab(), obligations.size());
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| top obligation", logger.tab());
			logger.log_indent++;
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| {}, [{}]", logger.tab(), n, join(state->cube));
			logger.log_indent--;

			if (frames.neg_inductive_rel_to(state->cube, n))
			{	//!s is now inductive to at least F_n
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| finishing state", logger.tab());
				logger.log_indent++;
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(), join(state->cube));
				logger.log_indent--;

				expr_vector core(ctx);
				// see if !state is also inductive relative to some m >= n
				int m = highest_inductive_frame(state->cube, n + 1, level, core);
				// m in [n-1, level]
				assert(static_cast<unsigned>(m+1) > n);

				if (m >= 0)
				{
					expr_vector smaller_state = generalize(core, m);
					// expr_vector smaller_state = generalize(state->cube, m);
					frames.remove_state(smaller_state, m + 1);
					obligations.erase(obligations.begin()); //problem with & structured binding??

					if (static_cast<unsigned>(m+1) <= level) //push upwards until inductive relative to F_level
					{
						SPDLOG_LOGGER_TRACE(logger.spd_logger, 
								"{}| push state to higher to level {}: [{}]", 
								logger.tab(), m+1, join(state->cube));
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
			else 
			{	// a !s predecessor found
				expr_vector pred_cube = frames.solver(n)->witness(
							[this](const expr& e) { return model.literals.atom_is_current(e); });
				std::shared_ptr<State> pred = std::make_shared<State>(pred_cube, state);

				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| predecessor found", logger.tab());
				logger.log_indent++;
				SPDLOG_LOGGER_TRACE(logger.spd_logger, "{}| [{}]", logger.tab(), join(pred->cube));
				logger.log_indent--;

				expr_vector core(ctx);
				// state is inductive relative to F[n-2]
				// else there would be an F[k-1] state t preceding state, making state an F[k-1] state
				int m = highest_inductive_frame(pred->cube, n-1, level, core);
				// m in [n-1, level]
				if (m >= 0)
				{
					expr_vector smaller_pred = generalize(core, m);
					frames.remove_state(smaller_pred, m + 1);

					if (static_cast<unsigned>(m+1) <= level)
					{
						SPDLOG_LOGGER_TRACE(logger.spd_logger, 
								"{}| push predecessor to level {}: [{}]", logger.tab(), m+1, join(pred->cube));
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
			logger.stats.obligation_done(level, elapsed);
			SPDLOG_LOGGER_TRACE(logger.spd_logger, "Obligation {} elapsed {}", branch, elapsed);
			std::cout << format("Obligation {} elapsed {}", branch, elapsed) << endl;
			elapsed = -1.0;

			if (period >= 100)
			{
				period = 0;
				std::cout << "Stats written" << endl;
				SPDLOG_LOGGER_DEBUG(logger.spd_logger, logger.stats.to_string());
				logger.spd_logger->flush();
			}
			else period++;
		}
		return true;
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
		out << fmt::format("Results pebbling strategy with {} pebbles for {}",
				model.get_max_pebbles(), model.name) << endl;
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
