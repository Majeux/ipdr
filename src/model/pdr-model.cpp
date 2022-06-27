#include "pdr-model.h"
#include "cli-parse.h"
#include <TextTable.h>
#include <numeric>
#include <optional>
#include <z3++.h>
#include <z3_api.h>

namespace pebbling
{
  using std::string;
  using z3::expr;
  using z3::expr_vector;

#warning pass NDEBUG flag for experiment runs
  Model::Model(z3::config& settings, const my::cli::ArgumentList& args,
      const dag::Graph& G)
      : ctx(settings), lits(ctx), property(ctx), n_property(ctx), initial(ctx),
        transition(ctx)
  {
    name = args.model_name;

    for (string node : G.nodes)
      lits.add_literal(node);
    lits.finish();
    // for (auto l : lits.currents())
    //   std::cout << l.id() << " ";
    // std::cout << std::endl;
    // for (auto l : lits.nexts())
    //   std::cout << l.id() << " ";
    // std::cout << std::endl;

    for (const expr& e : lits.currents())
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

  const expr_vector& Model::get_transition() const { return transition; }

  const expr_vector& Model::get_initial() const { return initial; }

  size_t Model::n_nodes() const { return initial.size(); }

  void Model::load_pebble_transition(const dag::Graph& G)
  {
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      string name = lits(i).to_string();
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const string& child : G.get_children(name))
      {
        expr child_node = ctx.bool_const(child.c_str());
        int child_i     = lits.indexof(child_node);
        // clang-format off
        transition.push_back( lits(i) || !lits.p(i) || lits(child_i));
        transition.push_back(!lits(i) ||  lits.p(i) || lits(child_i));
        transition.push_back( lits(i) || !lits.p(i) || lits.p(child_i));
        transition.push_back(!lits(i) ||  lits.p(i) || lits.p(child_i));
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
      expr stay        = add_and(transition, stay_name, lits(n), lits.p(n));
      stay_expr.emplace(n, stay);
    }

    expr_vector moves(ctx);
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      string name      = lits(i).to_string();
      string flip_name = fmt::format("_flip[{}]_", name);
      expr flip        = add_xor(transition, flip_name, lits(i), lits.p(i));
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
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      string name      = lits(i).to_string();
      expr parent_flip = lits(i) ^ lits.p(i);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      for (const string& child : G.get_children(name))
      {
        expr child_node    = ctx.bool_const(child.c_str());
        int child_i        = lits.indexof(child_node);
        expr child_pebbled = lits(child_i) & lits.p(child_i);

        transition.push_back(z3::implies(parent_flip, child_pebbled));
      }
    }
  }

  void Model::load_pebble_transition_raw2(const dag::Graph& G)
  {
    for (int i = 0; i < lits.size(); i++) // every node has a transition
    {
      string name      = lits(i).to_string();
      expr parent_flip = lits(i) ^ lits.p(i);
      // pebble if all children are pebbled now and next
      // or unpebble if all children are pebbled now and next
      expr_vector children_pebbled(ctx);
      for (const string& child : G.get_children(name))
      {
        expr child_node = ctx.bool_const(child.c_str());
        int child_i     = lits.indexof(child_node);
        children_pebbled.push_back(lits(child_i));
        children_pebbled.push_back(lits.p(child_i));
      }
      transition.push_back(
          z3::implies(parent_flip, z3::mk_and(children_pebbled)));
    }
  }

  void Model::load_property(const dag::Graph& G)
  {
    // final nodes are pebbled and others are not
    for (const expr& e : lits.currents())
    {
      if (G.is_output(e.to_string()))
        n_property.add_expression(e, lits);
      else
        n_property.add_expression(!e, lits);
    }
    n_property.finish();

    // final nodes are unpebbled and others are
    expr_vector disjunction(ctx);
    for (const expr& e : lits.currents())
    {
      if (G.is_output(e.to_string()))
        disjunction.push_back(!e);
      else
        disjunction.push_back(e);
    }
    property.add_expression(z3::mk_or(disjunction), lits);
    property.finish();
  }

  expr_vector Model::constraint(std::optional<unsigned> x)
  {
    expr_vector v(ctx);
    if (!x)
      return v;

    v.push_back(z3::atmost(lits.currents(), *x));
    v.push_back(z3::atmost(lits.nexts(), *x));

    return v;
  }

  unsigned Model::get_f_pebbles() const { return final_pebbles; }

  void Model::show(std::ostream& out) const
  {
    using std::endl;
    lits.show(out);
    unsigned t_size = std::accumulate(transition.begin(), transition.end(), 0,
        [](unsigned acc, const expr& e) { return acc + e.num_args(); });

    out << fmt::format("Transition Relation (size = {}):", t_size) << endl
        << transition << endl;
    out << "property: " << endl;
    property.show(out);
    out << "not_property: " << endl;
    n_property.show(out);
  }
} // namespace pebbling
