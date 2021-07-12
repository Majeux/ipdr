#ifndef PDR_MODEL
#define PDR_MODEL

#include <z3++.h>
#include <memory>
#include <vector>
#include <string>

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
		std::string name;
		int max_pebbles;
		shared_ptr<context> ctx;
		ExpressionCache literals;
		ExpressionCache property;
		ExpressionCache not_property;

		PDRModel(shared_ptr<context> c);
		void load_model(const std::string& model_name, const Graph& G, int max_pebbles);
		const expr_vector& get_transition() const;
		const expr_vector& get_initial() const;
		const expr_vector& get_cardinality() const;

	private:
		expr_vector initial;
		expr_vector transition; //vector of clauses (cnf)
		expr_vector cardinality; //cardinality constraint for current and next state

		void load_pebble_transition(const Graph& G);
		void load_property(const Graph& G);
};

#endif // !PDR_MODEL
