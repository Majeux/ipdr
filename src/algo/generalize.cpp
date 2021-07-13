#include <cstddef>
#include <z3++.h>
#include <algorithm>
#include <vector>

#include "pdr.h"
#include "z3-ext.h"
#include "string-ext.h"

using z3::expr_vector;
using Z3extensions::expr_less;
using Z3extensions::negate;
using Z3extensions::convert;
using str::extensions::join;

int PDR::highest_inductive_frame(const expr_vector& cube, int min, int max)
{
	expr clause = z3::mk_or(negate(cube)); //negate cube via demorgan
	expr_vector cube_p = model.literals.p(cube);

	// if SAT(F_0 & !s & T & s') aka F_0 & !s & T /=> !s'
	if (min <= 0 && frames[0]->SAT(clause, cube_p))
	{
		frames[0]->discard_model();
		return -1; //intersects with D[0]
	}

	int highest = max;
	for (int i = std::max(1, min); i <= max; i++)
	{
		//cube was inductive up to this iteration
		if (frames[i]->SAT(clause, cube_p))
		{
			frames[i]->discard_model();
			highest = i - 1; //previous was greatest inductive frame
			break;
		}
	}

	log->trace("{}| highest inductive frame is {}", TAB, highest);
	return highest;
}

expr_vector PDR::generalize(const expr_vector& state, int level)
{
	log->trace("{}| generalize", TAB);
	log_indent++;
	expr_vector smaller_cube = MIC(state, level);
	log_indent--;

	log->trace("{}| final reduced cube = [{}]", TAB, join(smaller_cube));
	return smaller_cube;
}


expr_vector PDR::MIC(const expr_vector& state, int level) 
{
	std::vector<expr> cube = convert(state); //use std::vector for sorting and intersection
	
	std::sort(cube.begin(), cube.end(), expr_less());

	for (unsigned i = 0; i < cube.size();) 
	{
		std::vector<expr> new_cube(cube.begin(), cube.begin()+i);
		new_cube.reserve(cube.size()-1);
		new_cube.insert(new_cube.end(), cube.begin()+i+1, cube.end());

		log_indent++;

		if (down(new_cube, level)) //current literal was dropped, i now points to the next
		{
			log_indent--;
			cube = std::move(new_cube);
			// log->trace("{}| reduced cube: [{}]", TAB, join(cube));
		}
		else 
		{
			i++;
			log_indent--;
		}
	}

	return convert(cube); //revert to expr_vector for efficiency in z3
}

//state is sorted
bool PDR::down(vector<expr>& state, int level)
{
	auto is_current_in_state = [this, &state](const expr& e)
	{
		return model.literals.literal_is_current(e) 
			&& std::binary_search(state.begin(), state.end(), e, expr_less());
	};

	while (true)
	{
		expr * const raw_state = state.data();
		if (init_solver.check(state.size(), raw_state) == z3::sat)
			return false;

		expr state_clause = z3::mk_or(negate(state));
		if( frames[level]->UNSAT(state_clause, model.literals.p(state)) )
			return true;
		
		//intersect the current states from the model with state
		vector<expr> cti_intersect; 
		frames[level]->sat_cube(cti_intersect,
				is_current_in_state,
				[&cti_intersect](size_t n) { cti_intersect.reserve(n); });

		state = move(cti_intersect);

		// log->trace("{}| down-reduction: [{}]", TAB, join(state));
	}
	return false;
}
