#include "pdr-model.h"
#include "cli-parse.h"
#include <TextTable.h>
#include <numeric>
#include <optional>
#include <z3++.h>
#include <z3_api.h>

namespace pdr
{
  using std::string;
  using z3::expr;

  IModel::IModel(z3::context& c, const std::vector<std::string>& varnames)
      : ctx(c),
        vars(ctx, varnames),
        property(ctx, vars),
        n_property(ctx, vars),
        initial(ctx),
        transition(ctx),
        constraint(ctx)
  {
  }

  const z3::expr_vector& IModel::get_initial() const { return initial; }
  const z3::expr_vector& IModel::get_transition() const { return transition; }
  const z3::expr_vector& IModel::get_constraint() const { return constraint; }

  void IModel::show(std::ostream& out) const
  {
    using std::endl;

    unsigned t_size = std::accumulate(transition.begin(), transition.end(), 0,
        [](unsigned acc, const z3::expr& e) { return acc + e.num_args(); });

    out << fmt::format("Transition Relation (size = {}):", t_size) << endl
        << transition << endl;

    out << endl << "property: " << endl;
    for (std::string_view s : property.names())
      out << s << endl;

    out << endl << "neg_property: " << endl;
    for (std::string_view s : n_property.names())
      out << s << endl;
  }

  //  Z3Model
  //
  Z3Model::Z3Model(z3::context& c, const std::vector<std::string>& varnames)
      : ctx(c), vars(ctx, varnames)
  {
  }

  Z3Model::~Z3Model() {}

  void Z3Model::show(std::ostream& out) const
  {
    out << to_string() << std::endl;
  }

  Z3Model::Rule Z3Model::mk_rule(expr const& e, string const& n)
  {
    return { forall_vars(e), ctx.str_symbol(n.c_str()) };
  }

  Z3Model::Rule Z3Model::mk_rule(
      expr const& head, expr const& body, string const& n)
  {
    return { forall_vars(z3::implies(body, head)), ctx.str_symbol(n.c_str()) };
  }

  expr Z3Model::forall_vars(expr const& e) const
  {
    return z3::forall(z3ext::vec_add(vars(), vars.p()), e);
  }
} // namespace pdr
