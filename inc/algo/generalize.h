#include <z3++.h>

#include "pdr.h"

using z3::expr_vector;

expr_vector PDR::generalize(const expr_vector& cube, int level)
{
	return cube;
}
