#include "pdr-model.h"
#include "cli-parse.h"
#include <TextTable.h>
#include <numeric>
#include <optional>
#include <z3++.h>
#include <z3_api.h>

namespace pdr
{
  const z3::expr_vector& IModel::get_transition() const { return transition; }
  const z3::expr_vector& IModel::get_initial() const { return initial; }

  void IModel::show(std::ostream& out) const
  {
    using std::endl;

    unsigned t_size = std::accumulate(transition.begin(), transition.end(), 0,
        [](unsigned acc, const z3::expr& e) { return acc + e.num_args(); });

    out << fmt::format("Transition Relation (size = {}):", t_size) << endl
        << transition << endl;
    out << "property: " << endl;
    property.show(out);
    out << "not_property: " << endl;
    n_property.show(out);
  }
} // namespace pebbling
