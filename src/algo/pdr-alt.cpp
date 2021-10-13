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
	// bool PDR::iterate_short()
	// {
	// 	cout << SEP3 << endl;
	// 	cout << "Start iteration" << endl;
	// 	extend_frames(1);

	// 	// I => P and I & T ⇒ P' (from init)
	// 	// continue until the frontier (F[i]) becomes a fixpoint
	// 	for (unsigned k = 1; ; k++)
	// 	{
	// 		cout << "iterate frame "<< k << endl;
	// 		SPDLOG_LOGGER_TRACE(log, "");
	// 		SPDLOG_LOGGER_TRACE(log, SEP3);
	// 		SPDLOG_LOGGER_TRACE(log, "{}| frame {}", TAB, k);
	// 		assert(k == frames.size() - 1);

	// 		while (true)
	// 		{
	// 			if (frames[k]->SAT(model.not_property.nexts()))
	// 			{
	// 					// F_i & T /=> F_i+1' (= P')
	// 				// strengthen F_i
	// 				SPDLOG_LOGGER_TRACE(log, "{}| cti found at frame {}", TAB, k);
	// 				cout << "new cti" << endl;
	// 				log_indent++;
					
	// 				expr_vector cti_current = frames[k]->sat_cube(
	// 						[this](const expr& e) { return model.literals.atom_is_current(e); });

	// 				SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(cti_current));	
	// 				log_indent--;
					
	// 				if (not block_short(cti_current, k-1, k)) 
	// 					return false;
	// 				log_indent--;
	// 				SPDLOG_LOGGER_TRACE(log, SEP2);
	// 				cout << endl;
	// 			}
	// 			else // no more counter examples
	// 				break;
	// 		}

	// 		SPDLOG_LOGGER_TRACE(log, "{}| propagate frame {} to {}", TAB, k, k+1);
	// 		log_indent++;

	// 		assert(frames.size() == k+1);
	// 		extend_frames(k+1);
	// 		if (propagate(k))
	// 			return true;

	// 		log_indent--;
	// 		cout << "###############" << endl;
	// 		SPDLOG_LOGGER_TRACE(log, SEP3); 
	// 		for (const unique_ptr<Frame>& f : frames)
	// 			SPDLOG_LOGGER_TRACE(log, "{}", (*f).solver_str());
	// 		SPDLOG_LOGGER_TRACE(log, SEP3);
	// 	}
	// }

	// bool PDR::block_short(expr_vector& cti, unsigned n, unsigned level)
	// {
	// 	unsigned period = 0;
	// 	std::set<Obligation, std::less<Obligation>> obligations; 
	// 	if ((n + 1) <= level)
	// 		obligations.emplace(n+1, std::move(cti), 0);

	// 	while (!obligations.empty())
	// 	{
	// 		sub_timer.reset();
	// 		double elapsed; string branch;

	// 		auto [n, state, depth] = *(obligations.begin());

	// 		assert(n <= level);
	// 		SPDLOG_LOGGER_TRACE(log, SEP);
	// 		SPDLOG_LOGGER_TRACE(log, "{}| obligations pending: {}", TAB, obligations.size());
	// 		SPDLOG_LOGGER_TRACE(log, "{}| top obligation", TAB);
	// 		log_indent++;
	// 		SPDLOG_LOGGER_TRACE(log, "{}| {}, [{}]", TAB, n, join(state->cube));
	// 		log_indent--;

	// 		expr state_clause = z3::mk_or(z3ext::negate(state->cube));

	// 		if ( frames[n]->SAT(state_clause, model.literals.p(state->cube)) )
	// 		{	//predecessor found
	// 			expr_vector pred_cube = frames[n]->sat_cube(
	// 						[this](const expr& e) { return model.literals.atom_is_current(e); });
	// 			shared_ptr<State> pred = std::make_shared<State>(pred_cube, state);

	// 			SPDLOG_LOGGER_TRACE(log, "{}| predecessor found", TAB);
	// 			log_indent++;
	// 			SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(pred->cube));
	// 			log_indent--;

	// 			if (n == 0)
	// 			{ 
	// 				bad = pred;
	// 				return false;
	// 			}

	// 			SPDLOG_LOGGER_TRACE(log, "{}| push predecessor to level {}: [{}]", TAB, n-1, join(pred->cube));
	// 			obligations.emplace(n-1, pred, depth+1);

	// 			elapsed = sub_timer.elapsed().count();
	// 			branch = "(pred)  ";
	// 		}
	// 		else 
	// 		{	//finish state
	// 			SPDLOG_LOGGER_TRACE(log, "{}| finishing state", TAB);
	// 			log_indent++;
	// 			SPDLOG_LOGGER_TRACE(log, "{}| [{}]", TAB, join(state->cube));
	// 			log_indent--;
				

	// 			expr_vector smaller_state = generalize(frames[n]->unsat_core(), n);
	// 			expr_vector core(ctx);
	// 			int m = highest_inductive_frame(state->cube, n+1, level);
	// 			assert(m > 0);

	// 			// expr_vector smaller_state = generalize(state->cube, m);
	// 			remove_state(smaller_state, m+1);
	// 			obligations.erase(obligations.begin()); //problem with & structured binding??

	// 			if (static_cast<unsigned>(m+1) <= level)
	// 			{
	// 				SPDLOG_LOGGER_TRACE(log, "{}| push state to higher to level {}: [{}]", TAB, m+1, join(state->cube));
	// 				obligations.emplace(m+1, state, depth);
	// 			}

	// 			elapsed = sub_timer.elapsed().count();
	// 			branch = "(finish)";
	// 		}
	// 		stats.obligation_done(level, elapsed);
	// 		SPDLOG_LOGGER_TRACE(log, "Obligation {} elapsed {}", branch, elapsed);
	// 		cout << format("Obligation {} elapsed {}", branch, elapsed) << endl;
	// 		elapsed = -1.0;

	// 		if (period >= 100)
	// 		{
	// 			period = 0;
	// 			cout << "Stats written" << endl;
	// 			SPDLOG_LOGGER_DEBUG(log, stats.to_string());
	// 			log->flush();
	// 		}
	// 		else period++;
	// 	}
	// 	return true;
	// }
}
