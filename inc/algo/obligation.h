#ifndef PDR_OBL_H
#define PDR_OBL_H

#include "TextTable.h"
#include "z3-ext.h"

#include <fmt/format.h>
#include <numeric>
#include <string>
#include <vector>
#include <z3++.h>

namespace pdr
{
  class State
  {
   public:
    z3::expr_vector cube;
    std::shared_ptr<State> prev; // store predecessor for trace

    State(const z3::expr_vector& e);
    State(const z3::expr_vector& e, std::shared_ptr<State> s);
    // move constructors
    State(z3::expr_vector&& e);
    State(z3::expr_vector&& e, std::shared_ptr<State> s);

    unsigned show(TextTable& table) const;

    unsigned no_marked() const;
  };

  namespace state
  {
    unsigned no_marked(const z3::expr_vector& s);
    std::vector<std::string> marking(
        const State& s, std::vector<std::string> header, unsigned width);
  } // namespace state

  struct Obligation
  {
    unsigned level;
    std::shared_ptr<State> state;
    unsigned depth;

    Obligation(unsigned k, z3::expr_vector&& cube, unsigned d);

    Obligation(unsigned k, const std::shared_ptr<State>& s, unsigned d);

    // bool operator<(const Obligation& o) const { return this->level <
    // o.level; }
    bool operator<(const Obligation& o) const;
  };
} // namespace pdr
#endif // PDR_OBL
