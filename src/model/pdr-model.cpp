#include "pdr-model.h"

using std::move;

PDRModel::PDRModel(shared_ptr<context> c) : 
	ctx(c), 
	literal_index(new unordered_map<unsigned, size_t>()),
	literals(new expr_vector(*c)),
	literals_p(new expr_vector(*c)),
	not_literals(new expr_vector(*c)),
	not_literals_p(new expr_vector(*c)),
	initial(*c),
	transition(*c), 
	cardinality(*c)
{ }

void PDRModel::load_pebble_transition(const Graph& G, int max_pebbles)
{
	for (size_t i = 0; i < literals->size(); i++) //every node has a transition
	{
		//pebble if all children are pebbled now and next
		//or unpebble if all children are pebbled now and next
		for (const string& child : G.children.at(literals[i].to_string()))
		{
			expr child_node = ctx->bool_const(child.c_str());
			size_t child_i = literal_index->at(child_node.hash());

			transition.push_back(    literals[i] | not_literals_p[i] | literals[child_i]);
			transition.push_back(not_literals[i] |     literals_p[i] | literals[child_i]);
			transition.push_back(    literals[i] | not_literals_p[i] | literals_p[child_i]);
			transition.push_back(not_literals[i] |     literals_p[i] | literals_p[child_i]);
		}
	}

	cardinality.push_back(z3::atmost(*literals, max_pebbles));
	cardinality.push_back(z3::atmost(*literals_p, max_pebbles));
}

void PDRModel::load_model(const Graph& G, int max_pebbles) 
{
	std::cout << "load graph model" << std::endl;

	for (string node : G.nodes)
	{
		expr lit = ctx->bool_const(node.c_str());
		expr lit_p = ctx->bool_const((node + ".p").c_str());
		expr not_lit = !lit;
		expr not_lit_p = !lit_p;

		literals->push_back(move(lit));
		literals_p->push_back(move(lit_p));
		not_literals->push_back(move(not_lit));
		not_literals_p->push_back(move(not_lit_p));

		literal_index->insert(std::make_pair(lit.hash(), literals->size()-1));
	}

	for (const expr& e : *not_literals)
		initial.push_back(e);

	load_pebble_transition(G, max_pebbles);
	load_property();
}

const expr& PDRModel::not(const expr& lit)
{
	size_t i = literal_index->at(lit.hash());
	return not_literals[i];
}

const expr& PDRModel::next(const expr& lit)
{
	size_t i = literal_index->at(lit.hash());
	return literals_p[i];
}

const expr& PDRModel::not_next(const expr& lit)
{
	size_t i = literal_index->at(lit.hash());
	return not_literals_p[i];
}