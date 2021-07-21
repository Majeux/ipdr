#include "pdr.h"
#include "z3-ext.h"
#include "string-ext.h"

#include <functional>
#include <iterator>
#include <cassert>
#include <algorithm>
#include <set>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>
#include <fmt/format.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fstream>
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

	Frame* PDR::make_frame(int level)
	{
		assert(level >= 0);
		SPDLOG_LOGGER_TRACE(log, "{}| creating new frame {}", TAB, level);

		if (level == 0)
			return new Frame(0, ctx, stats, { model.get_initial(), model.get_transition(), model.get_cardinality() });

		return new Frame(level, ctx, stats, { model.property.currents(), model.get_transition(), model.get_cardinality() }, log);
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

	 bool PDR::run()
	{
		timer.reset();
		bool failed = false;
		cout << endl << "PDR start:" << endl;
		SPDLOG_LOGGER_INFO(log, "");
		SPDLOG_LOGGER_INFO(log, "NEW RUN\n");
		SPDLOG_LOGGER_INFO(log, "PDR start");
		
		SPDLOG_LOGGER_INFO(log, "Start initiation");
		log_indent++;
		failed = !init();
		log_indent--;

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
		cout << format("Total elapsed time {}", timer) << endl;
		SPDLOG_LOGGER_INFO(log, "Total elapsed time {}", timer);
		log_indent = 0;

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
		frames.emplace_back(make_frame(0));

		if ( frames[0]->SAT(model.not_property.nexts()) )
		{
			std::cout << "I & T =/> P'" << std::endl;
			expr_vector bad_cube = frames[0]->sat_cube(
					[this](const expr& e) { return model.literals.atom_is_current(e); });
			bad = std::make_shared<State>(bad_cube);

			return false;
		}

		return true;
	}

	bool PDR::iterate()
	{
		cout << SEP3 << endl;
		cout << "Start iteration" << endl;
		frames.emplace_back(make_frame(1));

		// I => P and I & T ⇒ P' (from init)
		// continue until the frontier (F[i]) becomes a fixpoint
		for (unsigned k = 1; ; k++)
		{
			cout << "iterate frame "<< k << endl;
			SPDLOG_LOGGER_TRACE(log, "");
			SPDLOG_LOGGER_TRACE(log, SEP3);
			SPDLOG_LOGGER_TRACE(log, "{}| frame {}", TAB, k);
			assert(k == frames.size() - 1);

			while (true)
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

			SPDLOG_LOGGER_TRACE(log, "{}| propagate frame {} to {}", TAB, k, k+1);
			log_indent++;

			frames.emplace_back(make_frame(k+1));
			if (propagate(k))
				return true;

			log_indent--;
			cout << "###############" << endl;
			SPDLOG_LOGGER_TRACE(log, SEP3); 
			for (const unique_ptr<Frame>& f : frames)
				SPDLOG_LOGGER_TRACE(log, "{}", (*f).solver_str());
			SPDLOG_LOGGER_TRACE(log, SEP3);
		}
	}

	bool PDR::iterate_short()
	{
		cout << SEP3 << endl;
		cout << "Start iteration" << endl;
		frames.emplace_back(make_frame(1));

		// I => P and I & T ⇒ P' (from init)
		// continue until the frontier (F[i]) becomes a fixpoint
		for (unsigned k = 1; ; k++)
		{
			cout << "iterate frame "<< k << endl;
			SPDLOG_LOGGER_TRACE(log, "");
			SPDLOG_LOGGER_TRACE(log, SEP3);
			SPDLOG_LOGGER_TRACE(log, "{}| frame {}", TAB, k);
			assert(k == frames.size() - 1);

			while (true)
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
					
					if (not block_short(cti_current, k-1, k)) 
						return false;
					log_indent--;
					SPDLOG_LOGGER_TRACE(log, SEP2);
					cout << endl;
				}
				else // no more counter examples
					break;
			}

			SPDLOG_LOGGER_TRACE(log, "{}| propagate frame {} to {}", TAB, k, k+1);
			log_indent++;

			assert(frames.size() == k+1);
			frames.emplace_back(make_frame(k+1));
			if (propagate(k))
				return true;

			log_indent--;
			cout << "###############" << endl;
			SPDLOG_LOGGER_TRACE(log, SEP3); 
			for (const unique_ptr<Frame>& f : frames)
				SPDLOG_LOGGER_TRACE(log, "{}", (*f).solver_str());
			SPDLOG_LOGGER_TRACE(log, SEP3);
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

			SPDLOG_LOGGER_TRACE(log, "SAT | assertions:\n {}", frames[n]->solver_str());
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

	bool PDR::block_short(expr_vector& cti, unsigned n, unsigned level)
	{
		unsigned period = 0;
		std::set<Obligation, std::less<Obligation>> obligations; 
		if ((n + 1) <= level)
			obligations.emplace(n+1, std::move(cti), 0);

		while (!obligations.empty())
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

				if (n == 0)
				{ 
					bad = pred;
					return false;
				}

				SPDLOG_LOGGER_TRACE(log, "{}| push predecessor to level {}: [{}]", TAB, n-1, join(pred->cube));
				obligations.emplace(n-1, pred, depth+1);

				elapsed = sub_timer.elapsed().count();
				branch = "(pred)  ";
			}
			else 
			{	//finish state
				SPDLOG_LOGGER_TRACE(log, "{}| finishing state", TAB);
				log_indent++;
				SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(state->cube));
				log_indent--;
				

				expr_vector smaller_state = generalize(frames[n]->unsat_core(), n);
				expr_vector core(ctx);
				int m = highest_inductive_frame(state->cube, n+1, level);
				assert(m > 0);

				// expr_vector smaller_state = generalize(state->cube, m);
				remove_state(smaller_state, m+1);
				obligations.erase(obligations.begin()); //problem with & structured binding??

				if (static_cast<unsigned>(m+1) <= level)
				{
					SPDLOG_LOGGER_TRACE(log, "{}| push state to higher to level {}: [{}]", TAB, m+1, join(state->cube));
					obligations.emplace(m+1, state, depth);
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
				SPDLOG_LOGGER_TRACE(log, "{}| alreadt in {}", TAB, i);
			}

		}
		log_indent--;
	}

	bool PDR::propagate(unsigned level)
	{
		assert(level + 1 == frames.size()-1);
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
					frames[i+1]->block_cube(cube);
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
			
			frames[i+1]->reset_solver();
		}
		SPDLOG_LOGGER_TRACE(log, "Propagation elapsed {}", sub_timer);
		cout << format("Propagation elapsed {}", sub_timer) << endl;
		return false;
	}

	void PDR::show_results(std::ostream& out) const
	{
		out << format("Results pebbling strategy with {} pebbles for {}", model.max_pebbles, model.name) << endl;
		out << SEP2 << endl;

		out << "Bad state reached:" << endl;
		out << format("[ {} ]", join_expr_vec(model.not_property.currents(), " & ")) << endl << endl;

		out << "Reached from:" << endl;
		show_trace(out);
		out << SEP2 << endl;

		out << "Frames" << endl;
		for (const unique_ptr<Frame>& f : frames)
				out << format("{}", (*f).blocked_str()) << endl;

		out << SEP << endl;
		out << "Solvers" << endl;
		for (const unique_ptr<Frame>& f : frames)
				out << format("{}", (*f).solver_str()) << endl;
	}

	void PDR::show_trace(std::ostream& out) const
	{
		out << format("{} |\t [ {} ]", 0, join_expr_vec(model.get_initial())) << endl;

		int i = 1;
		shared_ptr<State> current = bad;
		while (current)
		{
			out << format("{} |\t [ {} ]", i, join_expr_vec(current->cube)) << endl; 
			current = current->prev;
			i++;
		}

		out << format("{} |\t [ {} ]", i, join_expr_vec(model.not_property.currents())) << endl;
	}
}
