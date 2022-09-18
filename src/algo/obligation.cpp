#include "obligation.h"

namespace pdr
{
  using std::shared_ptr;
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  // STATE MEMBERS
  //
  State::State(const expr_vector& e) : cube(e), prev(shared_ptr<State>()) {}
  State::State(const expr_vector& e, shared_ptr<State> s) : cube(e), prev(s) {}
  // move constructors
  State::State(expr_vector&& e) : cube(std::move(e)), prev(shared_ptr<State>())
  {
  }
  State::State(expr_vector&& e, shared_ptr<State> s)
      : cube(std::move(e)), prev(s)
  {
  }

  vector<string> State::marking(vector<string> header, unsigned width) const
  {
    vector<string> rv(header.size(), "?");
    for (const expr& e : cube)
    {
      string s = e.is_not() ? e.arg(0).to_string() : e.to_string();
      auto it  = std::lower_bound(header.begin(), header.end(), s);
      if (it != header.end() && *it == s) // it points to s
      {
        string fill_X           = fmt::format("{:X^{}}", "", width);
        rv[it - header.begin()] = e.is_not() ? "" : fill_X;
      }
    }

    return rv;
  }

  unsigned State::show(TextTable& table) const
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
    steps.emplace_back(i, z3ext::join_expr_vec(cube), count_pebbled(cube));

    shared_ptr<State> current = prev;
    while (current)
    {
      i++;
      steps.emplace_back(
          i, z3ext::join_expr_vec(current->cube), count_pebbled(current->cube));
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

  // MISC FUNCTIONS
  //
  unsigned no_marked(const z3::expr_vector& ev)
  {
    return std::accumulate(ev.begin(), ev.end(), 0,
        [](unsigned x, const z3::expr& e) { return x + (!e.is_not()); });
  }

  unsigned no_marked(const pdr::State& s) { return no_marked(s.cube); }

  // OBLIGATION MEMBERS
  //
  Obligation::Obligation(unsigned k, expr_vector&& cube, unsigned d)
      : level(k), state(std::make_shared<State>(std::move(cube))), depth(d)
  {
  }

  Obligation::Obligation(unsigned k, const shared_ptr<State>& s, unsigned d)
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
