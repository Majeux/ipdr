#include <TextTable.h>
#include <numeric>
#include <optional>
#include <z3++.h>
#include <z3_api.h>

#include "cli-parse.h"
#include "pebbling-model.h"

namespace pdr::pebbling
{
  using std::pair;
  using std::string;
  using z3::expr;
  using z3::expr_vector;

  Model::Model(z3::config& settings, const my::cli::ArgumentList& args,
      const dag::Graph& G)
      : IModel(settings, G.nodes)
  {
    name = args.model_name;

    for (const expr& e : vars())
      initial.push_back(!e);

    // load_pebble_transition_raw2(G);
    if (args.tseytin)
      load_pebble_transition_tseytin(G);
    else
      load_pebble_transition(G);

    final_pebbles = G.output.size();
    load_property(G);

    // z3::solver s(ctx);
    // z3::param_descrs p = s.get_param_descrs();
    // TextTable param_out(' ');
    // for (unsigned i = 0; i < p.size(); i++)
    // {
    //   z3::symbol sym = p.name(i);
    //   Z3_param_kind kind = p.kind(sym);
    //   string doc = p.documentation(sym);
    //   std::vector<std::string> row = {sym.str(), doc};
    //   param_out.addRow(row);
    // }
    //   std::cout << param_out << std::endl;
  }

  size_t Model::n_nodes() const { return initial.size(); }

  void Model::load_pebble_transition(const dag::Graph& G)
  {
    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      string name = vars(i).to_string();
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const string& child : G.get_children(name))
      {
        expr child_node   = ctx.bool_const(child.c_str());
        expr child_node_p = vars.p(child_node_p);
        // clang-format off
        transition.push_back( vars(i) || !vars.p(i) || child_node);
        transition.push_back(!vars(i) ||  vars.p(i) || child_node);
        transition.push_back( vars(i) || !vars.p(i) || child_node_p);
        transition.push_back(!vars(i) ||  vars.p(i) || child_node_p);
        // clang-format on
      }
    }
  }

  void Model::load_pebble_transition_tseytin(const dag::Graph& G)
  {
    using namespace z3ext::tseytin;
    transition.resize(0);
    // ((pv,i ^ pv,i+1 ) => (pw,i & pw,i+1 ))
    std::map<string, expr> stay_expr;
    for (const string& n : G.nodes)
    {
      string stay_name = fmt::format("_stay[{}]_", n);
      expr curr        = ctx.bool_const(n.c_str());
      expr next        = vars.p(curr);

      expr stay = add_and(transition, stay_name, curr, next);
      stay_expr.emplace(n, stay);
    }

    expr_vector moves(ctx);
    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      string name      = vars(i).to_string();
      string flip_name = fmt::format("_flip[{}]_", name);
      expr flip        = add_xor(transition, flip_name, vars(i), vars.p(i));
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const string& child : G.get_children(name))
      {
        expr child_stay = stay_expr.at(child);
        string move_str = fmt::format("_flip[{}] => stay[{}]_", name, child);
        expr move       = add_implies(transition, move_str, flip, child_stay);
        moves.push_back(move);
      }
    }
    for (const expr& e : moves)
      transition.push_back(e);
  }

  void Model::load_pebble_transition_raw1(const dag::Graph& G)
  {
    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      string name      = vars(i).to_string();
      expr parent_flip = vars(i) ^ vars.p(i);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const string& child : G.get_children(name))
      {
        expr child_node    = ctx.bool_const(child.c_str());
        expr child_node_p  = vars.p(child_node);
        expr child_pebbled = child_node & child_node_p;

        transition.push_back(z3::implies(parent_flip, child_pebbled));
      }
    }
  }

  void Model::load_pebble_transition_raw2(const dag::Graph& G)
  {
    for (size_t i = 0; i < vars().size(); i++) // every node has a transition
    {
      string name      = vars(i).to_string();
      expr parent_flip = vars(i) ^ vars.p(i);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      expr_vector children_pebbled(ctx);
      for (const string& child : G.get_children(name))
      {
        expr child_node = ctx.bool_const(child.c_str());
        expr child_node_p  = vars.p(child_node);
        children_pebbled.push_back(child_node);
        children_pebbled.push_back(child_node_p);
      }
      transition.push_back(
          z3::implies(parent_flip, z3::mk_and(children_pebbled)));
    }
  }

  void Model::load_property(const dag::Graph& G)
  {
    // final nodes are pebbled and others are not
    for (const expr& e : vars())
    {
      if (G.is_output(e.to_string()))
        n_property.add(e);
      else
        n_property.add(!e);
    }
    n_property.finish();

    // final nodes are unpebbled and others are
    expr_vector disjunction(ctx);
    for (const expr& e : vars())
    {
      if (G.is_output(e.to_string()))
        disjunction.push_back(!e);
      else
        disjunction.push_back(e);
    }
    property.add(z3::mk_or(disjunction));
    property.finish();
  }

  pair<expr_vector, expr_vector> Model::make_constraint(std::optional<unsigned> x)
  {
    pair rv = { expr_vector(ctx), expr_vector(ctx) };
    if (!x)
      return rv;

    rv.first.push_back(z3::atmost(vars, *x));
    rv.second.push_back(z3::atmost(vars.p(), *x));

    return rv;
  }

  unsigned Model::get_f_pebbles() const { return final_pebbles; }
} // namespace pebbling
