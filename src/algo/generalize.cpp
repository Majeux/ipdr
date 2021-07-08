#include <cstddef>
#include <z3++.h>
#include <algorithm>
#include <vector>

#include "pdr.h"

using z3::expr_vector;

expr_vector negate(const expr_vector& cube) 
{
	expr_vector negated(cube.ctx());
	for (const expr& e : cube)
		negated.push_back(!e);
	return negated;
}

expr_vector convert(vector<expr> vec) 
{
	expr_vector convert(vec[0].ctx());
	for (const expr& e : vec)
		convert.push_back(move(e));
	return convert;
}

vector<expr> convert(const expr_vector& vec) 
{
	vector<expr> convert; convert.reserve(vec.size());
	for (const expr& e : vec)
		convert.push_back(e);
	return convert;
}

int PDR::highest_inductive_frame(const expr_vector& cube, int min, int max)
{
	expr clause = z3::mk_or(negate(cube)); //negate cube via demorgan
	expr_vector cube_p = model.literals.p(cube);

	// if SAT(F_0 & !s & T & s') aka F_0 & !s & T /=> !s'
	if (min <= 0 && frames[0]->SAT(clause, cube_p))
		return -1; //intersects with D[0]

	for (int i = std::max(1, min); i <= max; i++)
	{
		//cube was inductive up to this iteration
		if (frames[i]->SAT(clause, cube_p))
			return i - 1; //previous was greatest inductive frame
	}

	return max;
}

expr_vector PDR::generalize(const expr_vector& state, int level)
{
	expr_vector smaller_cube = MIC(state, level);

	return state;
}

expr_vector PDR::MIC(const expr_vector& state, int level) 
{
	unsigned size = state.size();
	std::vector<expr> cube = convert(state); //use std::vector for sorting and intersection
	
	std::sort(cube.begin(), cube.end(), 
			[](const expr& l, const expr& r) { return l.id() < r.id(); });

	for (unsigned i = 0; i < size;) 
	{
		std::vector<expr> new_cube(cube.begin(), cube.begin()+i);
		new_cube.reserve(size);
		new_cube.insert(new_cube.end(), cube.begin()+i, cube.end());

		if (down(new_cube, level)) //current literal was dropped, i now points to the next
			cube = std::move(new_cube);
		else 
			i++;
	}

	return convert(cube); //revert to expr_vector for efficiency in z3
}

//state is sorted
bool PDR::down(vector<expr>& state, int level)
{
	while (true)
	{
		expr * const raw_state = state.data();
		if (init_solver.check(state.size(), raw_state) == z3::sat)
			return false;

		expr state_clause = z3::mk_or(convert(state));
		if( frames[level]->UNSAT(state_clause, model.literals.p(state)) == z3::unsat )
			return true;

		//TODO check if hash_set is faster
		vector<expr> cti_current; 
		frames[level]->sat_cube(cti_current,
				[this](const expr& e) { return model.literals.is_current(e); },
				[&cti_current](size_t n) { cti_current.reserve(n); });

		std::sort(cti_current.begin(), cti_current.end(), 
			[](const expr& l, const expr& r) { return l.id() < r.id(); });

		vector<expr> inter; inter.reserve(state.size());
		//inter := state AND cti_current
		std::set_intersection( 
				state.begin(), state.end(),
				cti_current.begin(), cti_current.end(),
				std::back_inserter(inter));
		state = move(inter);

	}
	return false;
}
