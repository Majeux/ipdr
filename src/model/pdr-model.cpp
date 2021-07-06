#include "pdr-model.h"

using std::move;

PDRModel::PDRModel(shared_ptr<context> c) : 
	ctx(c), 
	literals(c),
	property(c),
	initial(*c),
	transition(*c), 
	cardinality(*c)
{ }

void PDRModel::load_pebble_transition(const Graph& G, int max_pebbles)
{
	for (int i = 0; i < literals.size(); i++) //every node has a transition
	{
		string name = literals(i).to_string();
		//pebble if all children are pebbled now and next
		//or unpebble if all children are pebbled now and next
		for (const string& child : G.children.at(name))
		{
			expr child_node = ctx->bool_const(child.c_str());
			int child_i = literals.indexof(child_node);

			transition.push_back(    literals(i) | literals.notp(i) | literals(child_i));
			transition.push_back(literals.not(i) |    literals.p(i) | literals(child_i));
			transition.push_back(    literals(i) | literals.notp(i) | literals.p(child_i));
			transition.push_back(literals.not(i) |    literals.p(i) | literals.p(child_i));
		}
	}

	cardinality.push_back(z3::atmost(literals.lits(), max_pebbles));
	cardinality.push_back(z3::atmost(literals.ps(), max_pebbles));

	std::cout << transition << std::endl;
	std::cout << cardinality << std::endl;
}

void PDRModel::load_property()
{

}

void PDRModel::load_model(const Graph& G, int max_pebbles) 
{
	std::cout << "load graph model" << std::endl;

	for (string node : G.nodes)
		literals.add_literal(node);

	for (const expr& e : literals.nots())
		initial.push_back(e);

	literals.print();

	load_pebble_transition(G, max_pebbles);
	load_property();
}