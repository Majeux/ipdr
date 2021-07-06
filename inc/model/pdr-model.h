#ifndef PDR_MODEL
#define PDR_MODEL

#include <z3++.h>
#include <memory>
#include <vector>

#include "dag.h"
#include "exp-cache.h"

using std::shared_ptr;
using z3::context;
using z3::expr_vector;

struct Clause
{

};

class PDRModel
{
	public:
		PDRModel(shared_ptr<context> c);

		void load_model(const Graph& G, int max_pebbles);
	
	private:
		shared_ptr<context> ctx;

		ExpressionCache literals;
		ExpressionCache property;
		ExpressionCache not_property;

		expr_vector initial;
		expr_vector transition; //vector of clauses (cnf)
		expr_vector cardinality; //cardinality constraint for current and next state

		void load_pebble_transition(const Graph& G, int max_pebbles);
		void load_property(const Graph& G);
};

#endif // !PDR_MODEL
