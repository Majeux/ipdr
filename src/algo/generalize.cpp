#include <cstddef>
#include <z3++.h>
#include <algorithm>
#include <vector>

#include "pdr.h"
#include "z3-ext.h"
#include "string-ext.h"

using z3::expr_vector;

int PDR::highest_inductive_frame(const expr_vector& cube, int min, int max)
{
	expr clause = z3::mk_or(z3ext::negate(cube)); //negate cube via demorgan
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
		//clause was inductive up to this iteration
		if (frames[i]->SAT(clause, cube_p))
		{
			frames[i]->discard_model();
			highest = i - 1; //previous was greatest inductive frame
			break;
		}
	}

	SPDLOG_LOGGER_TRACE(log, "{}| highest inductive frame is {}", TAB, highest);
	return highest;
}

int PDR::highest_inductive_frame(const expr_vector& cube, int min, int max, expr_vector& core)
{
	int result = highest_inductive_frame(cube, min, max);
	if (result >= 0)
	{
		// F_result & !cube & T & cube' == UNSAT
		// F_result & !cube & T & core' == UNSAT
		core = frames[result]->unsat_core(
				[this](const expr& e) { return model.literals.literal_is_p(e); },
				[this](const expr& e) { return model.literals(e); });
		// std::cout << "core: " << join(core) << std::endl;
		// if I => !core, the subclause survives initiation and is inductive
		if (init_solver.check(core) == z3::sat) //I /=> !core
			core = cube; // core is not inductive, use original
	}
	SPDLOG_LOGGER_TRACE(log, "{}| used substituted cube for core: {} -> {}", TAB, cube.size(), core.size()); 
	log_indent++;
	// SPDLOG_LOGGER_TRACE(log, "{}| [ {} ]", TAB, join(cube));
	// SPDLOG_LOGGER_TRACE(log, "{}| [ {} ]", TAB, join(core));
	log_indent--;

	return result;
}

expr_vector PDR::generalize(const expr_vector& state, int level)
{
	SPDLOG_LOGGER_TRACE(log, "{}| generalize", TAB);
	log_indent++;
	expr_vector smaller_cube = MIC(state, level);
	log_indent--;

	SPDLOG_LOGGER_TRACE(log, "{}| generalized cube: {} -> {}", TAB, state.size(), smaller_cube.size());
	// SPDLOG_LOGGER_TRACE(log, "{}| final reduced cube = [{}]", TAB, join(smaller_cube));
	return smaller_cube;
}


expr_vector PDR::MIC(const expr_vector& state, int level) 
{
	std::vector<expr> cube = z3ext::convert(state); //use std::vector for sorting and intersection
	
	std::sort(cube.begin(), cube.end(), z3ext::expr_less());

	unsigned attempts = 0;
	for (unsigned i = 0; i < cube.size() && attempts < mic_retries;) 
	{
		std::vector<expr> new_cube(cube.begin(), cube.begin()+i);
		new_cube.reserve(cube.size()-1);
		new_cube.insert(new_cube.end(), cube.begin()+i+1, cube.end());

		log_indent++;

		if (down(new_cube, level)) //current literal was dropped, i now points to the next
		{
			log_indent--;
			cube = std::move(new_cube);
			attempts = 0;
			// SPDLOG_LOGGER_TRACE(log, "{}| reduced cube: [{}]", TAB, join(cube));
		}
		else 
		{
			i++;
			attempts++;
			log_indent--;
		}
	}

	return z3ext::convert(cube); //revert to expr_vector for efficiency in z3
}

//state is sorted
bool PDR::down(vector<expr>& state, int level)
{
	auto is_current_in_state = [this, &state](const expr& e)
	{
		return model.literals.literal_is_current(e) 
			&& std::binary_search(state.begin(), state.end(), e, z3ext::expr_less());
	};

	while (true)
	{
		expr * const raw_state = state.data();
		if (init_solver.check(state.size(), raw_state) == z3::sat)
			return false;

		expr state_clause = z3::mk_or(z3ext::negate(state));
		if( frames[level]->UNSAT(state_clause, model.literals.p(state)) )
			return true;
		
		//intersect the current states from the model with state
		vector<expr> cti_intersect; 
		frames[level]->sat_cube(cti_intersect,
				is_current_in_state,
				[&cti_intersect](size_t n) { cti_intersect.reserve(n); });

		state = move(cti_intersect);

		// SPDLOG_LOGGER_TRACE(log, "{}| down-reduction: [{}]", TAB, join(state));
	}
	return false;
}
