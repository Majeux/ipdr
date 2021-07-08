#include "z3-ext.h"

using std::vector;
using z3::expr;
using z3::expr_vector;

namespace Z3extensions
{
	//z3::expr comparator
	bool expr_less(const expr& l, const expr& r) { return l.id() < r.id(); };

	expr_vector negate(const expr_vector& cube) 
	{
		expr_vector negated(cube.ctx());
		for (const expr& e : cube)
			negated.push_back(!e);
		return negated;
	}

	expr_vector convert(vector<expr> vec) 
	{
		expr_vector converted(vec[0].ctx());
		for (const expr& e : vec)
			converted.push_back(std::move(e));
		return converted;
	}

	vector<expr> convert(const expr_vector& vec) 
	{
		vector<expr> converted; converted.reserve(vec.size());
		for (const expr& e : vec)
			converted.push_back(e);
		return converted;
	}
}
