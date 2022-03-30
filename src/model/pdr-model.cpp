#include "pdr-model.h"
#include <TextTable.h>
#include <numeric>
#include <z3++.h>
#include <z3_api.h>

namespace pdr
{
  PebblingModel::PebblingModel(z3::config& settings,
      const std::string& model_name, const dag::Graph& G, int pebbles)
      : ctx(settings), lits(ctx), property(ctx), n_property(ctx), initial(ctx),
        transition(ctx), cardinality(ctx)
  {
    name = model_name;

    for (std::string node : G.nodes)
      lits.add_literal(node);
    lits.finish();

    for (const z3::expr& e : lits.currents())
      initial.push_back(!e);

    // load_pebble_transition_raw2(G);
    load_pebble_transition(G);
    // load_pebble_transition_tseytin(G);

    final_pebbles = G.output.size();
    set_max_pebbles(pebbles);

    load_property(G);

    // z3::solver s(ctx);
    // z3::param_descrs p = s.get_param_descrs();
    // TextTable param_out(' ');
    // for (unsigned i = 0; i < p.size(); i++)
    // {
    //   z3::symbol sym = p.name(i);
    //   Z3_param_kind kind = p.kind(sym);
    //   std::string doc = p.documentation(sym);
    //   std::vector<std::string> row = {sym.str(), doc};
    //   param_out.addRow(row);
    // }
    //   std::cout << param_out << std::endl;
  }

  const z3::expr_vector& PebblingModel::get_transition() const
  {
    return transition;
  }
  const z3::expr_vector& PebblingModel::get_initial() const { return initial; }
  const z3::expr_vector& PebblingModel::get_cardinality() const
  {
    return cardinality;
  }
  size_t PebblingModel::n_nodes() const { return initial.size(); }

  void PebblingModel::load_pebble_transition(const dag::Graph& G)
  {
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      std::string name = lits(i).to_string();
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const std::string& child : G.get_children(name))
      {
        z3::expr child_node = ctx.bool_const(child.c_str());
        int child_i         = lits.indexof(child_node);

        transition.push_back(lits(i) || !lits.p(i) || lits(child_i));
        transition.push_back(!lits(i) || lits.p(i) || lits(child_i));
        transition.push_back(lits(i) || !lits.p(i) || lits.p(child_i));
        transition.push_back(!lits(i) || lits.p(i) || lits.p(child_i));
      }
    }
  }

  void PebblingModel::load_pebble_transition_tseytin(const dag::Graph& G)
  {
    transition.resize(0);
    // ((pv,i ^ pv,i+1 ) => (pw,i & pw,i+1 ))
    std::map<std::string, z3::expr> stay_expr;
    for (const std::string& n : G.nodes)
    {
      std::string stay_name = fmt::format("__stay({})", n);
      z3::expr stay = tseytin_and(transition, stay_name, lits(n), lits.p(n));
      stay_expr.emplace(n, stay);
    }

    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      std::string name      = lits(i).to_string();
      std::string flip_name = fmt::format("__flip({})", name);
      z3::expr flip = tseytin_xor(transition, flip_name, lits(i), lits.p(i));
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const std::string& child : G.get_children(name))
      {
        z3::expr child_stay  = stay_expr.at(child);
        std::string move_str = fmt::format("__move({}, {})", name, child);
        z3::expr move = tseytin_implies(transition, move_str, flip, child_stay);
        transition.push_back(move);
      }
    }
  }

  void PebblingModel::load_pebble_transition_raw1(const dag::Graph& G)
  {
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      std::string name     = lits(i).to_string();
      z3::expr parent_flip = lits(i) ^ lits.p(i);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const std::string& child : G.get_children(name))
      {
        z3::expr child_node    = ctx.bool_const(child.c_str());
        int child_i            = lits.indexof(child_node);
        z3::expr child_pebbled = lits(child_i) & lits.p(child_i);

        transition.push_back(z3::implies(parent_flip, child_pebbled));
      }
    }
  }

  void PebblingModel::load_pebble_transition_raw2(const dag::Graph& G)
  {
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      std::string name     = lits(i).to_string();
      z3::expr parent_flip = lits(i) ^ lits.p(i);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      z3::expr_vector children_pebbled(ctx);
      for (const std::string& child : G.get_children(name))
      {
        z3::expr child_node = ctx.bool_const(child.c_str());
        int child_i         = lits.indexof(child_node);
        children_pebbled.push_back(lits(child_i));
        children_pebbled.push_back(lits.p(child_i));
      }
      transition.push_back(
          z3::implies(parent_flip, z3::mk_and(children_pebbled)));
    }
  }

  void PebblingModel::load_property(const dag::Graph& G)
  {
    // final nodes are pebbled and others are not
    for (const z3::expr& e : lits.currents())
    {
      if (G.is_output(e.to_string()))
        n_property.add_expression(e, lits);
      else
        n_property.add_expression(!e, lits);
    }
    n_property.finish();

    // final nodes are unpebbled and others are
    z3::expr_vector disjunction(ctx);
    for (const z3::expr& e : lits.currents())
    {
      if (G.is_output(e.to_string()))
        disjunction.push_back(!e);
      else
        disjunction.push_back(e);
    }
    property.add_expression(z3::mk_or(disjunction), lits);
    property.finish();
  }

  int PebblingModel::get_max_pebbles() const { return max_pebbles; }

  void PebblingModel::set_max_pebbles(int x)
  {
    max_pebbles = x;

    cardinality = z3::expr_vector(ctx);
    if (max_pebbles >= 0)
    {
      cardinality.push_back(z3::atmost(lits.currents(), max_pebbles));
      cardinality.push_back(z3::atmost(lits.nexts(), max_pebbles));
    }
  }

  int PebblingModel::get_f_pebbles() const { return final_pebbles; }

  void PebblingModel::show(std::ostream& out) const
  {
    lits.show(out);
    unsigned t_size = std::accumulate(transition.begin(), transition.end(), 0,
        [](unsigned acc, const z3::expr& e) { return acc + e.num_args(); });

    out << fmt::format("Transition Relation (size = {}):", t_size) << std::endl
        << transition << std::endl;
    out << "property: " << std::endl;
    property.show(out);
    out << "not_property: " << std::endl;
    n_property.show(out);
  }

  // c = a & b <=> (!a | !b | c) & (a | !c) & (b | !c)
  z3::expr PebblingModel::tseytin_and(z3::expr_vector& sub_defs,
      const std::string& name, const z3::expr& a, const z3::expr& b)
  {
    z3::expr c = ctx.bool_const(name.c_str());
    sub_defs.push_back(!a || !b || c);
    sub_defs.push_back(a || !c);
    sub_defs.push_back(b || !c);
    return c;
  }

  // c = a | b <=> (a | b | !c) & (!a | c) & (!b | c)
  z3::expr PebblingModel::tseytin_or(z3::expr_vector& sub_defs,
      const std::string& name, const z3::expr& a, const z3::expr& b)
  {
    z3::expr c = ctx.bool_const(name.c_str());
    sub_defs.push_back(a || b || !c);
    sub_defs.push_back(!a || c);
    sub_defs.push_back(!b || c);
    return c;
  }

  // a => b <=> !a | b
  z3::expr PebblingModel::tseytin_implies(z3::expr_vector& sub_defs,
      const std::string& name, const z3::expr& a, const z3::expr& b)
  {
    return tseytin_or(sub_defs, name, !a, b);
  }

  // c = a ^ b <=> (!a | !b | !c) & (a | b | !c) & (a | !b | c) & (!a | b | c)
  z3::expr PebblingModel::tseytin_xor(z3::expr_vector& sub_defs,
      const std::string& name, const z3::expr& a, const z3::expr& b)
  {
    z3::expr c = ctx.bool_const(name.c_str());
    sub_defs.push_back(!a || !b || !c);
    sub_defs.push_back(a || b || !c);
    sub_defs.push_back(a || !b || c);
    sub_defs.push_back(!a || b || c);
    return c;
  }
} // namespace pdr
