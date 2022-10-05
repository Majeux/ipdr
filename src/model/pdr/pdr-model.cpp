#include "pdr-model.h"
#include "cli-parse.h"
#include <TextTable.h>
#include <numeric>
#include <optional>
#include <z3++.h>
#include <z3_api.h>

namespace pdr
{
  IModel::IModel(const std::set<std::string>& varnames)
      : ctx(), vars(ctx, varnames), property(ctx, vars), n_property(ctx, vars),
        initial(ctx), transition(ctx), constraint(ctx)
  {
    ctx.set("unsat_core", true);
    ctx.set("model", true);
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
} // namespace pdr
