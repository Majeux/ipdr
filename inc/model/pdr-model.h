#ifndef PDR_MODEL
#define PDR_MODEL

#include <z3++.h>
#include <memory>
#include <vector>
#include <string>

#include "dag.h"
#include "exp-cache.h"

struct Clause
{

};

class PDRModel
{
	public:
		std::string name;
		z3::context ctx;
		ExpressionCache literals;
		ExpressionCache property;
		ExpressionCache not_property;

		PDRModel();
		PDRModel(z3::config& settings);
		void load_model(const std::string& model_name, const dag::Graph& G, int max_pebbles);
		const z3::expr_vector& get_transition() const;
		const z3::expr_vector& get_initial() const;
		const z3::expr_vector& get_cardinality() const;
		int get_max_pebbles() const;
		void set_max_pebbles(int x);

	private:
		int max_pebbles = -1;
		int final_pebbles = -1;

		z3::expr_vector initial;
		z3::expr_vector transition; //vector of clauses (cnf)
		z3::expr_vector cardinality; //cardinality constraint for current and next state

		void load_pebble_transition(const dag::Graph& G);
		void load_pebble_transition_raw1(const dag::Graph& G);
		void load_pebble_transition_raw2(const dag::Graph& G);
		void load_property(const dag::Graph& G);
};

#endif // !PDR_MODEL
