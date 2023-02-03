#include "obligation.h"
#include "result.h"
#include "z3-ext.h"
#include <algorithm>

namespace pdr
{
  using std::shared_ptr;
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using TraceState = PdrResult::Trace::TraceState;

  // STATE MEMBERS
  //
  PdrState::PdrState(const expr_vector& e)
      : cube(e), prev(shared_ptr<PdrState>())
  {
  }
  PdrState::PdrState(const expr_vector& e, shared_ptr<PdrState> s)
      : cube(e), prev(s)
  {
  }
  // move constructors
  PdrState::PdrState(expr_vector&& e)
      : cube(std::move(e)), prev(shared_ptr<PdrState>())
  {
  }
  PdrState::PdrState(expr_vector&& e, shared_ptr<PdrState> s)
      : cube(std::move(e)), prev(s)
  {
  }

  unsigned PdrState::show(TextTable& table) const
  {
    vector<std::tuple<unsigned, string, unsigned>> steps;

    auto count_pebbled = [](const expr_vector& vec)
    {
      unsigned count = 0;
      for (const expr& e : vec)
        if (!e.is_not())
          count++;

      return count;
    };

    unsigned i = 1;
    steps.emplace_back(i, z3ext::join_ev(cube), count_pebbled(cube));

    shared_ptr<PdrState> current = prev;
    while (current)
    {
      i++;
      steps.emplace_back(
          i, z3ext::join_ev(current->cube), count_pebbled(current->cube));
      current = current->prev;
    }
    unsigned i_padding = i / 10 + 1;

    string line_form = "{:>{}} |\t [ {} ] No. pebbled = {}";
    for (const auto& [num, vec, count] : steps)
    {
      vector<string> step_row = { std::to_string(num), vec,
        std::to_string(count) };
      table.addRow(step_row);
    }

    return i_padding;
  }

  // OBLIGATION MEMBERS
  //
  Obligation::Obligation(unsigned k, expr_vector&& cube, unsigned d)
      : level(k), state(std::make_shared<PdrState>(std::move(cube))), depth(d)
  {
  }

  Obligation::Obligation(unsigned k, const shared_ptr<PdrState>& s, unsigned d)
      : level(k), state(s), depth(d)
  {
  }

  // bool operator<(const Obligation& o) const { return this->level <
  // o.level; }
  bool Obligation::operator<(const Obligation& o) const
  {
    if (this->level < o.level)
      return true;
    if (this->level > o.level)
      return false;
    if (this->depth < o.depth)
      return true;
    if (this->depth > o.depth)
      return false;

    return z3ext::expr_vector_less()(this->state->cube, o.state->cube);
  }
} // namespace pdr
