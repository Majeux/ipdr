#ifndef Z3_EXT
#define Z3_EXT

#include <z3++.h>
#include <vector>

namespace Z3extensions
{
	//z3::expr comparator
	struct expr_less 
	{
		bool operator() (const z3::expr& l, const z3::expr& r) const { return l.id() < r.id(); };
	};

	z3::expr_vector negate(const z3::expr_vector& cube);
	z3::expr_vector convert(std::vector<z3::expr> vec);
	std::vector<z3::expr> convert(const z3::expr_vector& vec);
}
#endif //Z3_EXT
