#include "pdr.h"
#include "z3-ext.h"
#include "string-ext.h"

#include <functional>
#include <iterator>
#include <cassert>
#include <algorithm>
#include <memory>
#include <set>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fstream>
#include <tuple>
#include <z3++.h>

namespace pdr
{
	using std::cout;
	using z3ext::join_expr_vec;

	PDR::PDR(PDRModel& m) : ctx(m.ctx), model(m), init_solver(ctx)
	{
		init_solver.add(m.get_initial());
		string log_file = m.name + ".log";
		log = spdlog::basic_logger_mt("pdr_logger", "logs/" + log_file);
		log->set_level(spdlog::level::trace);
		// spdlog::flush_every(std::chrono::seconds(20));
		spdlog::flush_on(spdlog::level::trace);
	}

	void PDR::reset()
	{
		log_indent = 0;
		bad = std::shared_ptr<State>();
		stats = Statistics();
		frames_string = "None";
		solvers_string = "None";
	}

	Frame* PDR::make_frame(unsigned level)
	{
		SPDLOG_LOGGER_TRACE(log, "{}| creating new frame {}", TAB, level);

		if (level == 0)
			return new Frame(0, ctx, stats, { model.get_initial(), model.get_transition(), model.get_cardinality() });

		return new Frame(level, ctx, stats, { model.property.currents(), model.get_transition(), model.get_cardinality() }, log);
	}

	void PDR::extend_frames(unsigned level)
	{
		assert(level == frames.size());

		// if (dynamic_cardinality && 0 < level && level < old_frames.size())
		// {
		// 	frames.emplace_back(std::move(old_frames[level]));
		// 	frames.back()->reset_frame(stats, { model.property.currents(), model.get_transition(), model.get_cardinality() });
		// 	SPDLOG_LOGGER_INFO(log, "Using old clauses from level {}", level);
		// }
		// else
			frames.emplace_back(make_frame(level));
	}

	void PDR::print_model(const z3::model& m)
	{
		cout << "model consts \{" << std::endl;
		for (unsigned i = 0; i < m.num_consts(); i++)
		{
			cout << "\t" << m.get_const_interp( m.get_const_decl(i) );
		}
		cout << "}" << endl;
	}

	bool PDR::run(bool dynamic)
	{
		dynamic_cardinality = dynamic;
		reset();
		timer.reset();

		if (k == 0)
			assert(frames.size() == 0);
		else assert(frames.size() == k+1);

		bool failed = false;
		cout << endl << "PDR start:" << endl;
		SPDLOG_LOGGER_INFO(log, "");
		SPDLOG_LOGGER_INFO(log, "NEW RUN\n");
		SPDLOG_LOGGER_INFO(log, "PDR start");
		
		if (!dynamic || k == 0)
		{
			SPDLOG_LOGGER_INFO(log, "Start initiation");
			log_indent++;
			failed = !init();
			log_indent--;
		}

		if (failed)
		{
			cout << "Failed initiation" << endl;
			SPDLOG_LOGGER_TRACE(log, "Failed initiation");
			return finish(false);
		}
		cout << "Survived initiation" << endl;
		SPDLOG_LOGGER_INFO(log, "Survived initiation");

		SPDLOG_LOGGER_INFO(log, "Start iteration");
		log_indent++;
		failed = !iterate();
		log_indent--;

		if(failed) 
		{
			cout << "Failed iteration" << endl;
			SPDLOG_LOGGER_TRACE(log, "Failed iteration");
			return finish(false);
		}

		cout << "Property verified" << endl;
		SPDLOG_LOGGER_INFO(log, "Property verified");
		return finish(true);
	}

	bool PDR::finish(bool result) 
	{
		double final_time = timer.elapsed().count();
		cout << format("Total elapsed time {}", final_time) << endl;
		SPDLOG_LOGGER_INFO(log, "Total elapsed time {}", final_time);
		stats.elapsed = final_time;

		store_frame_strings();
		if (dynamic_cardinality)
			store_frames();

		return result;
	}

	// returns true if the model survives initiation
	bool PDR::init() 
	{
		SPDLOG_LOGGER_TRACE(log, "Start initiation");
		if ( init_solver.check(model.not_property.currents()) == z3::sat )
		{
			std::cout << "I =/> P" << std::endl;
			z3::model counter = init_solver.get_model();
			print_model(counter);
			//TODO TRACE	
			bad = std::make_shared<State>(model.get_initial());
			return false;
		}

		frames.clear();
		extend_frames(0);

		if ( frames[0]->SAT(model.not_property.nexts()) )
		{
			std::cout << "I & T =/> P'" << std::endl;
			expr_vector bad_cube = frames[0]->sat_cube(
					[this](const expr& e) { return model.literals.atom_is_current(e); });
			bad = std::make_shared<State>(bad_cube);

			return false;
		}

		extend_frames(1);
		k = 1;

		return true;
	}

	bool PDR::iterate()
	{
		cout << SEP3 << endl;
		cout << "Start iteration" << endl;

		// I => P and I & T ⇒ P' (from init)
		while (true) //iterate over k, if dynamic this continues from last k
		{
			cout << "iterate frame "<< k << endl;
			SPDLOG_LOGGER_TRACE(log, "");
			SPDLOG_LOGGER_TRACE(log, SEP3);
			SPDLOG_LOGGER_TRACE(log, "{}| frame {}", TAB, k);
			assert(k == frames.size() - 1);

			while (true) //exhaust all transitions to !P
			{
				if (frames[k]->SAT(model.not_property.nexts()))
				{
						// F_i & T /=> F_i+1' (= P')
					// strengthen F_i
					SPDLOG_LOGGER_TRACE(log, "{}| cti found at frame {}", TAB, k);
					cout << "new cti" << endl;
					log_indent++;
					
					expr_vector cti_current = frames[k]->sat_cube(
							[this](const expr& e) { return model.literals.atom_is_current(e); });

					SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(cti_current));	
					log_indent--;

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

					SPDLOG_LOGGER_TRACE(log, "{}| block", TAB);
					log_indent++;
					
					if (not block(cti_current, n+1, k)) 
						return false;
					log_indent--;
					SPDLOG_LOGGER_TRACE(log, SEP2);
					cout << endl;
				}
				else // no more counter examples
					break;
			}

			SPDLOG_LOGGER_TRACE(log, "{}| propagate frame {} to {}", TAB, 1, k);
			log_indent++;

			extend_frames(k+1);
			bool  done = propagate(k);
			if (k == 6)
			{
				cout << "SIX TIME\n//////////////////////////////" << endl;
			}
			k++;
			
			log_indent--;
			cout << "###############" << endl;
			SPDLOG_LOGGER_TRACE(log, SEP3); 
			for (const unique_ptr<Frame>& f : frames)
			{
				// if (k == 7) 
				// {
				// 	std::fstream fs;
				// 	fs.open ("test/dyn10-6-7.txt", std::fstream::in | std::fstream::app);
				// 	fs << (*f).solver_str() << endl;
				// 	fs.close();

				// }
				SPDLOG_LOGGER_TRACE(log, "{}", (*f).solver_str());
			}
			SPDLOG_LOGGER_TRACE(log, SEP3);

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
			SPDLOG_LOGGER_TRACE(log, SEP);
			SPDLOG_LOGGER_TRACE(log, "{}| obligations pending: {}", TAB, obligations.size());
			SPDLOG_LOGGER_TRACE(log, "{}| top obligation", TAB);
			log_indent++;
			SPDLOG_LOGGER_TRACE(log, "{}| {}, [{}]", TAB, n, join(state->cube));
			log_indent--;

			expr state_clause = z3::mk_or(z3ext::negate(state->cube));

			if ( frames[n]->SAT(state_clause, model.literals.p(state->cube)) )
			{	//predecessor found
				expr_vector pred_cube = frames[n]->sat_cube(
							[this](const expr& e) { return model.literals.atom_is_current(e); });
				shared_ptr<State> pred = std::make_shared<State>(pred_cube, state);

				SPDLOG_LOGGER_TRACE(log, "{}| predecessor found", TAB);
				log_indent++;
				SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(pred->cube));
				log_indent--;

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
						SPDLOG_LOGGER_TRACE(log, "{}| push predecessor to level {}: [{}]", TAB, m+1, join(pred->cube));
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
				SPDLOG_LOGGER_TRACE(log, "{}| finishing state", TAB);
				log_indent++;
				SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(state->cube));
				log_indent--;

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
						SPDLOG_LOGGER_TRACE(log, "{}| push state to higher to level {}: [{}]", TAB, m+1, join(state->cube));
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
			SPDLOG_LOGGER_TRACE(log, "Obligation {} elapsed {}", branch, elapsed);
			cout << format("Obligation {} elapsed {}", branch, elapsed) << endl;
			elapsed = -1.0;

			if (period >= 100)
			{
				period = 0;
				cout << "Stats written" << endl;
				SPDLOG_LOGGER_DEBUG(log, stats.to_string());
				log->flush();
			}
			else period++;
		}
		return true;
	}

	void PDR::remove_state(expr_vector& cube, int level)
	{
		level = std::min(static_cast<size_t>(level), frames.size()-1);
		SPDLOG_LOGGER_TRACE(log, "{}| removing cube from level [1..{}]: [{}]", TAB, level, join(cube));
		log_indent++;

		for (int i = 1; i <= level; i++)
		{
			if (frames[i]->block_cube(cube))
			{
				SPDLOG_LOGGER_TRACE(log, "{}| blocked in {}", TAB, i);
			}
			else 
			{
			}

		}
		log_indent--;
	}

	bool PDR::propagate(unsigned level, bool repeat)
	{
		assert(level == frames.size()-2); // k == |F|-1
		sub_timer.reset();
		cout << "propagate level " << level << endl;
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
							cout << "new blocked in repeat" << endl;
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

			if (diff.size() == 0 || frames[i]->equals(*frames[i + 1]))
			{
				cout << "Frame[" << i << "] == Frame[" << (i + 1) << "]" << endl;
				return true;
			}
			// if (diff.size() == 0)
			// {
			// 	cout << "Frame[" << i << "] \\ Frame[" << (i + 1) << "] = 0" << endl;
			// 	if (frames[i]->equals(*frames[i + 1]))
			// 		cout << "and are equal" << endl;
			// 	return true;
			// }
			// if (frames[i]->equals(*frames[i + 1]))
			// {
			// 	cout << "Frame[" << i << "] == Frame[" << (i + 1) << "]" << endl;
			// 	cout << "but diff != 0" << endl;
			// 	return true;
			// }
			
			
			frames[i+1]->reset_solver();
		}
		SPDLOG_LOGGER_TRACE(log, "Propagation elapsed {}", sub_timer);
		cout << format("Propagation elapsed {}", sub_timer) << endl;
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
