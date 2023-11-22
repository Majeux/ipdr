#include "pdr-model.h"
#include "cli-parse.h"
#include "result.h"

#include <numeric>
#include <optional>
#include <tabulate/table.hpp>
#include <z3++.h>
#include <z3_api.h>

namespace pdr
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::implies;
  using z3::mk_and;

  IModel::IModel(z3::context& c, const vector<string>& varnames)
      : ctx(c),
        vars(ctx, varnames),
        property(ctx, vars),
        n_property(ctx, vars),
        state(ctx),
        initial(ctx),
        transition(ctx),
        constraint(ctx),
        state_sorts(ctx)
  {
  }

  //
  const expr_vector& IModel::get_initial() const { return initial; }
  const expr_vector& IModel::get_transition() const { return transition; }
  const expr_vector& IModel::get_constraint() const { return constraint; }

  // fixedpoint interface
  //
  namespace
  {
    expr cube_to_assignment(expr_vector const& cube)
    {
      expr z3_true  = cube.ctx().bool_val(true);
      expr z3_false = cube.ctx().bool_val(false);

      expr_vector rv(cube.ctx());
      for (expr const& e : cube)
      {
        if (!z3ext::is_lit(e))
          throw std::runtime_error("Need a cube to turn into assignment.");
        rv.push_back(e.is_not() ? e.arg(0) == z3_false : e == z3_true);
      }
      return mk_and(rv);
    }

    // gather all expressions in e, except those in basic
    void aux_var_rec(expr const& e,
        std::set<expr, z3ext::expr_less>& visited,
        expr_vector& aux,
        std::set<expr, z3ext::expr_less> const& basic)
    {
      // already visited or is one the basic variables
      if (visited.insert(e).second == false || basic.find(e) != basic.end())
        return;

      if (e.is_const())
      {
        aux.push_back(e);
      }
      else if (e.is_app())
      {
        for (size_t i{ 0 }; i < e.num_args(); i++)
          aux_var_rec(e.arg(i), visited, aux, basic);
      }
      else if (e.is_quantifier())
      {
        aux_var_rec(e.body(), visited, aux, basic);
      }
    }

    // gather all auxiliary variables in e (introduced by tseitin.
    // return vars() + vars.p() + aux_vars
    expr_vector get_all_vars(expr e, expr_vector const& basic)
    {
      expr_vector aux(basic);
      std::set<expr, z3ext::expr_less> visited, basic_set;
      for (expr const& e : basic)
        basic_set.insert(e);

      aux_var_rec(e, visited, aux, basic_set);

      return z3ext::vec_add(basic, aux);
    }
  } // namespace

  void IModel::load_initial(z3::fixedpoint& engine)
  {
    state_sorts.resize(0);
    for (size_t i{ 0 }; i < state_size(); i++)
      state_sorts.push_back(ctx.bool_sort());

    state = z3::function("state", state_sorts, ctx.bool_sort());
    engine.register_relation(state);
    fp_I =
        mk_rule(implies(cube_to_assignment(get_initial()), state(vars())), "I");
    engine.add_rule(fp_I->expr, fp_I->name);
  }

  void IModel::load_transition(z3::fixedpoint& engine)
  {
    expr guard = get_constraint_current();
    expr trans = mk_and(get_transition());
    expr horn  = implies(state(vars()) && trans && guard, state(vars.p()));
    // collect potential auxiliary vars not in the state (such as from tseytin)
    expr_vector all_vars = get_all_vars(horn, z3ext::vec_add(vars(), vars.p()));

    fp_T.push_back({ z3::forall(all_vars, horn), ctx.str_symbol("T") });
    engine.add_rule(fp_T.back().expr, fp_T.back().name);
  }

  expr IModel::create_fp_target()
  {
    return z3::exists(vars(), state(vars) && mk_and(n_property()));
  }

  z3::func_decl& IModel::fp_query_ref() { return state; }

  vector<expr> extract_trace_states(z3::fixedpoint& engine)
  {
    vector<expr> rv;
    // answer {
    //  arg(0):
    //  arg(1):
    //  arg(2): destination
    // }
    expr answer = engine.get_answer().arg(0).arg(1);

    while (answer.num_args() == 3)
    {
      assert(answer.arg(2).get_sort().is_bool());
      rv.push_back(answer.arg(2));

      answer = answer.arg(1);
    }

    assert(answer.num_args() == 2);
    rv.push_back(answer.arg(1));
    std::reverse(rv.begin(), rv.end());
    return rv;
  }

  PdrResult::Trace::TraceVec IModel::fp_trace_states(z3::fixedpoint& engine)
  {
    using tabulate::Table;
    using z3ext::LitStr;
    using namespace str::ext;

    vector<vector<LitStr>> rv;
    const vector<string> header = vars.names();
    auto answer                 = engine.get_answer().arg(0).arg(1);

    vector<expr> states = extract_trace_states(engine);

    auto invalid = [](std::string_view s)
    {
      return std::invalid_argument(
          fmt::format("\"{}\" is not a valid state in the trace", s));
    };

    // use regex to extract the assignments of "true" and "false" from a state
    // they are in order of ts.vars.names()
    const std::regex marking(R"(\(state((?:\s+(?:true|false))*)\))");
    std::smatch match;

    for (size_t i{ 0 }; i < states.size(); i++)
    {
      string state_str(states[i].to_string());
      if (!std::regex_match(state_str, match, marking))
        throw invalid(state_str);
      assert(match.size() == 2);

      string mark_string = match[1];
      trim(mark_string); // remove white spaces at beginning and end
      // replace all white spaces by " "
      mark_string = std::regex_replace(mark_string, std::regex("\\s+"), " ");

      vector<string> marks = split(mark_string, ' ');

      assert(marks.size() == header.size());
      vector<LitStr> state;
      for (size_t j{ 0 }; j < marks.size(); j++)
      {
        if (marks[j] == "true")
          state.emplace_back(header.at(j), true);
        else if (marks[j] == "false")
          state.emplace_back(header.at(j), false);
        else
          throw invalid(state_str);
      }
      rv.push_back(state);
    }

    return rv;
  }

  void IModel::show(std::ostream& out) const
  {
    using std::endl;

    unsigned t_size = std::accumulate(transition.begin(), transition.end(), 0,
        [](unsigned acc, const expr& e) { return acc + e.num_args(); });

    out << fmt::format("Transition Relation (size = {}):", t_size) << endl
        << transition << endl;

    out << endl << "property: " << endl;
    for (std::string_view s : property.names())
      out << s << endl;

    out << endl << "neg_property: " << endl;
    for (std::string_view s : n_property.names())
      out << s << endl;
  }

  IModel::Rule IModel::mk_rule(expr const& e, string const& n)
  {
    return { forall_vars(e), ctx.str_symbol(n.c_str()) };
  }

  IModel::Rule IModel::mk_rule(
      expr const& head, expr const& body, string const& n)
  {
    return { forall_vars(implies(body, head)), ctx.str_symbol(n.c_str()) };
  }

  IModel::Rule IModel::mk_rule_aux(
      expr const& head, expr const& body, string const& n)
  {
    expr horn            = implies(body, head);
    expr_vector all_vars = get_all_vars(horn, z3ext::vec_add(vars(), vars.p()));
    return { z3::forall(all_vars, horn), ctx.str_symbol(n.c_str()) };
  }

  expr IModel::forall_vars(expr const& e) const
  {
    return z3::forall(z3ext::vec_add(vars(), vars.p()), e);
  }

} // namespace pdr
