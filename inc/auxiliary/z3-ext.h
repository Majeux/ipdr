#ifndef Z3_EXT
#define Z3_EXT

#include <z3++.h>
#include <vector>
#include <sstream>

using std::vector;
using std::string;
using z3::expr;
using z3::expr_vector;

namespace Z3extensions
{
	//z3::expr comparator
	struct expr_less 
	{
		bool operator() (const z3::expr& l, const z3::expr& r) const { return l.id() < r.id(); };
	};

	inline expr_vector negate(const expr_vector& lits) 
	{
		expr_vector negated(lits.ctx());
		for (const expr& e : lits)
			negated.push_back(!e);
		return negated;
	}

	inline expr_vector negate(const vector<expr>& lits) 
	{
		expr_vector negated(lits[0].ctx());
		for (const expr& e : lits)
			negated.push_back(!e);
		return negated;
	}

	inline expr_vector convert(vector<expr> vec) 
	{
		expr_vector converted(vec[0].ctx());
		for (const expr& e : vec)
			converted.push_back(std::move(e));
		return converted;
	}

	inline vector<expr> convert(const expr_vector& vec) 
	{
		vector<expr> converted; converted.reserve(vec.size());
		for (const expr& e : vec)
			converted.push_back(e);
		return converted;
	}
}
#endif //Z3_EXT
