#include "pdr-model.h"

using z3::expr;

PDRModel::PDRModel(shared_ptr<context> c) : 
	ctx(c), 
	literals(c),
	property(c),
	not_property(c),
	initial(*c),
	transition(*c), 
	cardinality(*c)
{ }

const expr_vector& PDRModel::get_transition() const { return transition; }
const expr_vector& PDRModel::get_initial() const { return initial; }
const expr_vector& PDRModel::get_cardinality() const { return cardinality; }

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

			transition.push_back(  literals(i) | !literals.p(i) | literals(child_i));
			transition.push_back( !literals(i) |  literals.p(i) | literals(child_i));
			transition.push_back(  literals(i) | !literals.p(i) | literals.p(child_i));
			transition.push_back( !literals(i) |  literals.p(i) | literals.p(child_i));
		}
	}

	cardinality.push_back(z3::atmost(literals.currents(), max_pebbles));
	cardinality.push_back(z3::atmost(literals.nexts(), max_pebbles));
}

void PDRModel::load_property(const Graph& G)
{
	//final nodes are pebbled and others are not
	for (const expr& e : literals.currents())
	{
		if (G.is_output(e.to_string()))
			not_property.add_expression(e, literals);
		else
			not_property.add_expression(!e, literals);
	}
	not_property.finish();

	//final nodes are unpebbled and others are
	expr_vector disjunction(*ctx);
	for (const expr& e : literals.currents())
	{
		if (G.is_output(e.to_string()))
			disjunction.push_back(!e);
		else
			disjunction.push_back(e);
	}
	property.add_expression(z3::mk_or(disjunction), literals);
	property.finish();
}

void PDRModel::load_model(const std::string& model_name, const Graph& G, int max_pebbles) 
{
	name = model_name;
	std::cout << "load graph model " << name << std::endl;

	for (string node : G.nodes)
		literals.add_literal(node);
	literals.finish();

	for (const expr& e : literals.currents())
		initial.push_back(!e);

	literals.print();

	load_pebble_transition(G, max_pebbles);
	std::cout << transition << std::endl;

	load_property(G);
	std::cout << "property: " << std::endl; property.print();
	std::cout << "not_property: " << std::endl; not_property.print();

}
