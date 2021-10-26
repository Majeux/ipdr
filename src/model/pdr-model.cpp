#include "pdr-model.h"
#include <z3++.h>

PDRModel::PDRModel()
    : ctx(), literals(ctx), property(ctx), n_property(ctx), initial(ctx),
      transition(ctx), cardinality(ctx)
{
}

PDRModel::PDRModel(z3::config& settings)
    : ctx(settings), literals(ctx), property(ctx), n_property(ctx),
      initial(ctx), transition(ctx), cardinality(ctx)
{
}

const z3::expr_vector& PDRModel::get_transition() const { return transition; }
const z3::expr_vector& PDRModel::get_initial() const { return initial; }
const z3::expr_vector& PDRModel::get_cardinality() const { return cardinality; }

void PDRModel::load_pebble_transition(const dag::Graph& G)
{
    for (int i = 0; i < literals.size(); i++) // every node has a transition
    {
        std::string name = literals(i).to_string();
        // pebble if all children are pebbled now and next
        // or unpebble if all children are pebbled now and next
        for (const std::string& child : G.children.at(name)) {
            z3::expr child_node = ctx.bool_const(child.c_str());
            int child_i = literals.indexof(child_node);

            transition.push_back(literals(i) || !literals.p(i) ||
                                 literals(child_i));
            transition.push_back(!literals(i) || literals.p(i) ||
                                 literals(child_i));
            transition.push_back(literals(i) || !literals.p(i) ||
                                 literals.p(child_i));
            transition.push_back(!literals(i) || literals.p(i) ||
                                 literals.p(child_i));
        }
    }
}

void PDRModel::load_pebble_transition_raw1(const dag::Graph& G)
{
    for (int i = 0; i < literals.size(); i++) // every node has a transition
    {
        std::string name = literals(i).to_string();
        z3::expr parent_flip = literals(i) ^ literals.p(i);
        // pebble if all children are pebbled now and next
        // or unpebble if all children are pebbled now and next
        for (const std::string& child : G.children.at(name)) {
            z3::expr child_node = ctx.bool_const(child.c_str());
            int child_i = literals.indexof(child_node);
            z3::expr child_pebbled = literals(child_i) & literals.p(child_i);

            transition.push_back(z3::implies(parent_flip, child_pebbled));
        }
    }
}

void PDRModel::load_pebble_transition_raw2(const dag::Graph& G)
{
    for (int i = 0; i < literals.size(); i++) // every node has a transition
    {
        std::string name = literals(i).to_string();
        z3::expr parent_flip = literals(i) ^ literals.p(i);
        // pebble if all children are pebbled now and next
        // or unpebble if all children are pebbled now and next
        z3::expr_vector children_pebbled(ctx);
        for (const std::string& child : G.children.at(name)) {
            z3::expr child_node = ctx.bool_const(child.c_str());
            int child_i = literals.indexof(child_node);
            children_pebbled.push_back(literals(child_i));
            children_pebbled.push_back(literals.p(child_i));
        }
        transition.push_back(
            z3::implies(parent_flip, z3::mk_and(children_pebbled)));
    }
}

void PDRModel::load_property(const dag::Graph& G)
{
    // final nodes are pebbled and others are not
    for (const z3::expr& e : literals.currents()) {
        if (G.is_output(e.to_string()))
            n_property.add_expression(e, literals);
        else
            n_property.add_expression(!e, literals);
    }
    n_property.finish();

    // final nodes are unpebbled and others are
    z3::expr_vector disjunction(ctx);
    for (const z3::expr& e : literals.currents()) {
        if (G.is_output(e.to_string()))
            disjunction.push_back(!e);
        else
            disjunction.push_back(e);
    }
    property.add_expression(z3::mk_or(disjunction), literals);
    property.finish();
}

void PDRModel::load_model(const std::string& model_name, const dag::Graph& G,
                          int pebbles)
{
    name = model_name;
    std::cout << "load graph model " << name << std::endl;

    for (std::string node : G.nodes)
        literals.add_literal(node);
    literals.finish();

    for (const z3::expr& e : literals.currents())
        initial.push_back(!e);

    literals.print();

    // load_pebble_transition_raw2(G);
    load_pebble_transition(G);
    std::cout << transition << std::endl;

    final_pebbles = G.output.size();
    set_max_pebbles(pebbles);

    load_property(G);
    std::cout << "property: " << std::endl;
    property.print();
    std::cout << "not_property: " << std::endl;
    n_property.print();
}

int PDRModel::get_max_pebbles() const { return max_pebbles; }

void PDRModel::set_max_pebbles(int x)
{
    assert(x >= final_pebbles);
    max_pebbles = x;

    cardinality = z3::expr_vector(ctx);
    cardinality.push_back(z3::atmost(literals.currents(), max_pebbles));
    cardinality.push_back(z3::atmost(literals.nexts(), max_pebbles));
}
