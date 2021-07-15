#include "frame.h"

Frame::Frame(int k, shared_ptr<context> c, const std::vector<expr_vector>& assertions) 
	: level(k), ctx(c), consecution_solver(*c/*, "QF_FD"*/)
{
	consecution_solver.set("sat.cardinality.solver", true);
	consecution_solver.set("cardinality.solver", true);
	// consecution_solver.set("lookahead_simplify", true);
	for (const expr_vector& v : assertions)
		consecution_solver.add(v);

	cubes_start = std::accumulate(assertions.begin(), assertions.end(), 
			0, [](int agg, const expr_vector& v) { return agg + v.size(); });
}

bool Frame::blocked(const expr& cube) const 
{ 
	return blocked_cubes.find(cube) != blocked_cubes.end(); 
}

bool Frame::block_cube(const expr_vector& cube)
{
	expr clause = z3::mk_or(negate(cube));
	bool inserted = blocked_cubes.insert(z3::mk_and(cube)).second;

	if (!inserted) //TODO possibly check some subsumption here
		return false;

	consecution_solver.add(clause);
	return true;
}

bool Frame::SAT(const expr& current, expr_vector next)
{
	next.push_back(current);
	return SAT(next);
}

bool Frame::SAT(const expr_vector& next) 
{ 
	if (consecution_solver.check(next) == z3::sat)
	{
		if(!model_used)
			std::cerr << "PDR::WARNING: last SAT model unused and discarded" << std::endl;
		model_used = false;
		return true;
	}
	return false;
}

bool Frame::UNSAT(const expr& current, expr_vector next) 
{ 
	next.push_back(current);
	return !SAT(next);
}

bool Frame::UNSAT(const expr_vector& next) 
{ 
	return !SAT(next);
}

void Frame::discard_model()
{
	model_used = true;
}

bool Frame::equals(const Frame& f) const
{ 
	return blocked_cubes == f.blocked_cubes;
}

std::vector<expr> Frame::diff(const Frame& f) const
{
	std::vector<expr> out;
	std::set_difference(
			blocked_cubes.begin(), blocked_cubes.end(),
			f.blocked_cubes.begin(), f.blocked_cubes.end(),
			std::back_inserter(out), expr_less());
	return out;
}

std::string Frame::solver_str() const
{
	std::string str(fmt::format("solver level {}\n", level));
	const expr_vector& asserts = consecution_solver.assertions();
	
	auto it = asserts.begin();
	for (int i = 0; i < cubes_start && it != asserts.end(); i++) it++;

	for (; it != asserts.end(); it++)
		str += fmt::format("- {}\n", (*it).to_string());

	return str;
}

std::string Frame::blocked_str() const
{
	std::string str(fmt::format("blocked cubes level {}\n", level));
	for (const expr& e : blocked_cubes)
		str += fmt::format("- {}\n", e.to_string());

	return str;
}
