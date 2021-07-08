#ifndef Z3_EXT
#define Z3_EXT

#include <z3++.h>
#include <vector>

namespace Z3extensions
{
	bool expr_less(const z3::expr& l, const z3::expr& r);
	z3::expr_vector negate(const z3::expr_vector& cube);
	z3::expr_vector convert(std::vector<z3::expr> vec);
	std::vector<z3::expr> convert(const z3::expr_vector& vec);
}
#endif //Z3_EXT
