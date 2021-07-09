#include <iterator>
#include <z3++.h>
#include <fmt/format.h>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <queue>
#include <set>

#include "pdr.h"
#include "z3-ext.h"

using std::cout;
using std::endl;
using std::set;
using fmt::format;
using Z3extensions::negate;

PDR::PDR(shared_ptr<context> c, const PDRModel& m) : ctx(c), model(m), init_solver(*c)
{
	init_solver.add(m.get_initial());

}

Frame* PDR::make_frame(int level)
{
	assert(level >= 0);

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

void PDR::run()
{
	cout << endl << "PDR start:" << endl;
	if (!init())
	{
		cout << "Failed initiation" << endl;
		return;
	}
	cout << "Survived initiation" << endl;

	if(!iterate()) 
	{
		cout << "Failed iteration" << endl;
		return;
	}
	cout << "Property verified" << endl;
}

// returns true if the model survives initiation
bool PDR::init() 
{

	if ( init_solver.check(model.not_property.currents()) == z3::sat )
	{
		std::cout << "I =/> P" << std::endl;
		z3::model counter = init_solver.get_model();
		print_model(counter);
		//TODO TRACE	
		return false;
	}

	frames.clear();
	frames.emplace_back(make_frame(0));

	if ( frames[0]->SAT(model.not_property.nexts()) )
	{
		std::cout << "I & T =/> P'" << std::endl;
		z3::model counter = init_solver.get_model();
		print_model(counter);
		//TODO TRACE	
		return false;
	}

	return true;
}

bool PDR::iterate()
{
	frames.emplace_back(make_frame(1));

	// I => P and I & T ⇒ P' (from init)
	// continue until the frontier (F[i]) becomes a fixpoint
	for (unsigned k = 1; ; k++)
	{
		cout << format("Frame {}:", k) << endl;
		// IC3Trace.LogLine("Frame " + k + ":");
		assert(k == frames.size() - 1);

		while (true)
		{
			if (frames[k]->SAT(model.not_property.nexts()))
			{
				// F_i & T /=> F_i+1' (= P')
				// strengthen F_i
				expr_vector cti_current(*ctx);
				frames[k]->sat_cube(cti_current,
						[this](const expr& e) { return model.literals.atom_is_current(e); });

				// cout << "cti: " <<  cti_current << endl;
				std::priority_queue<Obligation> obligations;

				// s is not in F_k-1 (or it would have been found previously)
				// F_k-2 & T & !s => !s'
				// only need to to search k-1 ... k
				int n = highest_inductive_frame(cti_current, (int)k - 1, (int)k);
				assert(n >= 0);

				// F_n & T & !s => !s
				// F_n & T => F_n+1
				expr_vector smaller_cti = generalize(cti_current, n);
				remove_state(smaller_cti, n + 1);

				if (static_cast<unsigned>(n + 1) <= k)
					obligations.emplace(n+1, cti_current);

				// IC3Trace.LogFunction("Block -- " + k);
				
				if (not block(obligations, k)) 
				{
					// trace.bad_state = NextFromModel(cti);
					// trace.to_bad = StateFromModel(cti);
					// trace.badI = counter;
					// trace.Frames = Frames;
					return false;
				}
				cout << endl;
			}
			else // no more counter examples
				break;
		}

		frames.emplace_back(make_frame(k+1));
		if (propagate(k))
			return true;

		cout << "###############" << endl;
	}
}

bool PDR::block(std::priority_queue<Obligation> obligations, unsigned level)
{
	while (obligations.size() > 0)
	{
		auto &[n, state] = obligations.top();
		assert(n <= level);
		expr state_clause = z3::mk_or(negate(state));

		if ( frames[level]->SAT(state_clause, model.literals.p(state)) )
		{	//predecessor found
			expr_vector pred(*ctx);
			frames[level]->sat_cube(pred,
						[this](const expr& e) { return model.literals.atom_is_current(e); });

			int m = highest_inductive_frame(pred, n-1, level);
			// m in [n-1, level]
			if (m >= 0)
			{
				expr_vector smaller_pred = generalize(pred, m);
				remove_state(smaller_pred, m + 1);

				if (static_cast<unsigned>(m+1) <= level)
				{
					obligations.emplace(m+1, pred);
				}
			}
			else //intersects with I
			{ 
				return false;
			}
		}
		else 
		{	//finish state
			int m = highest_inductive_frame(state, n + 1, level);
        	// m in [n-1, level]
			assert(static_cast<unsigned>(m+1) > n);

			if (m >= 0)
			{
				expr_vector smaller_state = generalize(state, m);
				remove_state(smaller_state, m + 1);
				obligations.pop();

				if (static_cast<unsigned>(m+1) <= level)
				{
					obligations.emplace(m+1, state);
				}
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

void PDR::remove_state(expr_vector& cube, int level)
{
	level = std::min(static_cast<size_t>(level), frames.size()-1);

	for (int i = 1; i <= level; i++)
	{
		frames[i]->block_cube(cube);

	}
}

bool PDR::propagate(unsigned level)
{
	assert(level + 1 == frames.size()-1);
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
		vector<expr> diff = (*frames[i]) - (*frames[i+1]);

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
		}

		if (diff.size() == 0 || (*frames[i]) == (*frames[i + 1]))
		{
			cout << "Frame[" << i << "] == Frame[" << (i + 1) << "]" << endl;
			return true;
		}
	}
	return false;
}
