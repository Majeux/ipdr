#ifndef PDR_MODEL
#define PDR_MODEL

#include <z3++.h>
#include <memory>
#include <vector>

#include "dag.h"

using std::unique_ptr;
using std::shared_ptr;
using std::unordered_map;
using z3::context;
using z3::expr;
using z3::expr_vector;

struct Clause
{

};

class PDRModel
{
	public:
		PDRModel(shared_ptr<context> c);

		void load_model(const Graph& G, int max_pebbles);

		const expr& not(const expr& lit);
		const expr& next(const expr& lit);
		const expr& not_next(const expr& lit);
	
	private:
		shared_ptr<context> ctx;

		shared_ptr<unordered_map<unsigned, size_t>> literal_index;
		shared_ptr<expr_vector> literals;
		shared_ptr<expr_vector> literals_p;
		shared_ptr<expr_vector> not_literals;
		shared_ptr<expr_vector> not_literals_p;

		expr_vector initial;
		expr_vector transition; //vector of clauses (cnf)
		expr_vector cardinality; //cardinality constraint for current and next state

		void load_pebble_transition(const Graph& G, int max_pebbles);
		void load_property();
};

#endif // !PDR_MODEL
