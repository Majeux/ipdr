#include <fmt/core.h>
#include <functional>
#include <iterator>
#include <cassert>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <fmt/format.h>
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <z3++.h>
#include <fstream>

#include "pdr.h"
#include "z3-ext.h"

using std::cout;
using std::endl;
using std::make_shared;
using Z3extensions::negate;
using fmt::format;

PDR::PDR(shared_ptr<context> c, const PDRModel& m, bool do_log) : ctx(c), model(m), init_solver(*c)
{
	init_solver.add(m.get_initial());
	// std::ofstream params_file("z3params/ctx.params");
	// params_file << init_solver.get_param_descrs() << std::endl;

	std::string log_file = m.name + ".log";
	log = spdlog::basic_logger_mt("pdr_logger", "logs/" + log_file);
	if (do_log)
	{
		cout << "logging on" << endl;
		log->set_level(spdlog::level::trace);
	}
	else
	{
		cout << "logging off" << endl;
		log->set_level(spdlog::level::off);
	}
	// log->flush_on(spdlog::level::trace);
}

Frame* PDR::make_frame(int level)
{
	assert(level >= 0);
	SPDLOG_LOGGER_TRACE(log, "{}| creating new frame {}", TAB, level);

	if (level == 0)
		return new Frame(0, model.ctx, { model.get_initial(), model.get_transition(), model.get_cardinality() });

	return new Frame(level, ctx, { model.property.currents(), model.get_transition(), model.get_cardinality() });
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
	log->info("");
	log->info("NEW RUN\n");
	log->info("PDR start");
	
	log->info("Start initiation");
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
	log->info("Survived initiation");

	log->info("Start iteration");
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
	log->info("Property verified");
	return finish(true);
}

bool PDR::finish(bool result) 
{
	cout << format("Total elapsed time {}", timer) << endl;
	log->info("Total elapsed time {}", timer);
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
		bad = std::make_shared<State>(expr_vector(*ctx));
		frames[0]->sat_cube(bad->cube,
				[this](const expr& e) { return model.literals.atom_is_current(e); });

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
				SPDLOG_LOGGER_TRACE(log, "{}| cti found", TAB);
				log_indent++;
				
				expr_vector cti_current(*ctx);
				frames[k]->sat_cube(cti_current,
						[this](const expr& e) { return model.literals.atom_is_current(e); });

				SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(cti_current));	
				log_indent--;

				std::priority_queue<MIN_ORDERING(Obligation)> obligations;
				// s is not in F_k-1 (or it would have been found previously)
				// F_k-2 & T & !s => !s'
				// only need to to search k-1 ... k
				expr_vector core(*ctx);
				int n = highest_inductive_frame(cti_current, (int)k - 1, (int)k, core);
				// int n = highest_inductive_frame(cti_current, (int)k - 1, (int)k);
				assert(n >= 0);

				// F_n & T & !s => !s
				// F_n & T => F_n+1
				expr_vector smaller_cti = generalize(core, n);
				// expr_vector smaller_cti = generalize(cti_current, n);

				remove_state(smaller_cti, n + 1);

				if (static_cast<unsigned>(n + 1) <= k)
					obligations.emplace(n+1, std::move(cti_current));

				SPDLOG_LOGGER_TRACE(log, "{}| block", TAB);
				log_indent++;
				
				if (not block(obligations, k)) 
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
		{
			SPDLOG_LOGGER_TRACE(log, "{}", (*f).solver_str());
		}
		SPDLOG_LOGGER_TRACE(log, SEP3);
	}
}

bool PDR::block(std::priority_queue<MIN_ORDERING(Obligation)> obligations, unsigned level)
{
	while (obligations.size() > 0)
	{
		sub_timer.reset();
		size_t start_size = obligations.size();
		auto &[n, state] = obligations.top();
		assert(n <= level);
		SPDLOG_LOGGER_TRACE(log, SEP);
		SPDLOG_LOGGER_TRACE(log, "{}| top obligations", TAB);
		log_indent++;
		SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(state->cube));
		log_indent--;

		expr state_clause = z3::mk_or(negate(state->cube));

		if ( frames[n]->SAT(state_clause, model.literals.p(state->cube)) )
		{	//predecessor found
			shared_ptr<State> pred = make_shared<State>(expr_vector(*ctx), state);
			frames[n]->sat_cube(pred->cube,
						[this](const expr& e) { return model.literals.atom_is_current(e); });

			SPDLOG_LOGGER_TRACE(log, "{}| predecessor found", TAB);
			log_indent++;
			SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(pred->cube));
			log_indent--;

			expr_vector core(*ctx);
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
					obligations.emplace(m+1, pred);
					assert(start_size+1 == obligations.size());
				}
				else
					assert(start_size == obligations.size());
			}
			else //intersects with I
			{ 
				bad = pred;
				return false;
			}
			SPDLOG_LOGGER_TRACE(log, "Obligation (pred)   elapsed {}", sub_timer);
			cout << format("Obligation (pred)   elapsed {}", sub_timer) << endl;
		}
		else 
		{	//finish state
			SPDLOG_LOGGER_TRACE(log, "{}| finishing state", TAB);
			log_indent++;
			SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(state->cube));
			log_indent--;

			expr_vector core(*ctx);
			int m = highest_inductive_frame(state->cube, n + 1, level, core);
			// int m = highest_inductive_frame(state->cube, n + 1, level);
        	// m in [n-1, level]
			assert(static_cast<unsigned>(m+1) > n);

			if (m >= 0)
			{
				expr_vector smaller_state = generalize(core, m);
				// expr_vector smaller_state = generalize(state->cube, m);
				remove_state(smaller_state, m + 1);
				obligations.pop();

				if (static_cast<unsigned>(m+1) <= level)
				{
					SPDLOG_LOGGER_TRACE(log, "{}| push state to higher to level {}: [{}]", TAB, m+1, join(state->cube));
					obligations.emplace(m+1, state);
					assert(start_size == obligations.size());
				}
				else
					assert(start_size-1 == obligations.size());
			}
			else
			{
				bad = state;
				return false;
			}
			SPDLOG_LOGGER_TRACE(log, "Obligation (finish) elapsed {}", sub_timer);
			cout << format("Obligation (finish) elapsed {}", sub_timer) << endl;
		}
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
		frames[i]->block_cube(cube);

	}
	log_indent--;
}

bool PDR::propagate(unsigned level)
{
	assert(level + 1 == frames.size()-1);
	sub_timer.reset();
	cout << "propagate level " << level << endl;
	//extracts arguments of e as an expr_vector
	auto extract = [this] (const expr& e) 
	{
		expr_vector result(*ctx);
		int size = e.num_args();
		for (int i = 0; i < size; i++)
			result.push_back(e.arg(i));
		return result;
	};

	for (unsigned i = 1; i <= level; i++)
	{   //TODO: check in of exclusief
		vector<expr> diff = frames[i]->diff(*frames[i+1]);

		for (const expr& c : diff)
		{
			expr_vector cube = extract(c);
			expr_vector cube_next = model.literals.p(cube);

			if (frames[i]->UNSAT(cube_next))
			{
				frames[i]->block_cube(cube);
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
	out << format("[ {} ]", join(model.not_property.currents(), " & ")) << endl << endl;

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
	out << format("{} |\t [ {} ]", 0, join(model.get_initial())) << endl;

	int i = 1;
	shared_ptr<State> current = bad;
	while (current)
	{
		out << format("{} |\t [ {} ]", i, join(current->cube)) << endl; 
		current = current->prev;
		i++;
	}

	out << format("{} |\t [ {} ]", i, join(model.not_property.currents())) << endl;
}
