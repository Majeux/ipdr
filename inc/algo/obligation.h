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
  class PdrState
  {
   public:
    z3::expr_vector cube;
    std::shared_ptr<PdrState> prev; // store predecessor for trace

    PdrState(const z3::expr_vector& e);
    PdrState(const z3::expr_vector& e, std::shared_ptr<PdrState> s);
    // move constructors
    PdrState(z3::expr_vector&& e);
    PdrState(z3::expr_vector&& e, std::shared_ptr<PdrState> s);

    unsigned show(TextTable& table) const;

    unsigned no_marked() const;
  };

  namespace state
  {
    unsigned no_marked(const z3::expr_vector& s);
    std::vector<std::string> marking(
        const PdrState& s, std::vector<std::string> header, unsigned width);
  } // namespace state

  struct Obligation
  {
    unsigned level;
    std::shared_ptr<PdrState> state;
    unsigned depth;

    Obligation(unsigned k, z3::expr_vector&& cube, unsigned d);

    Obligation(unsigned k, const std::shared_ptr<PdrState>& s, unsigned d);

    // bool operator<(const Obligation& o) const { return this->level <
    // o.level; }
    bool operator<(const Obligation& o) const;
  };
} // namespace pdr
#endif // PDR_OBL
