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
    std::vector<z3::expr> cube;
    std::shared_ptr<PdrState> prev; // store predecessor for trace

    PdrState(const std::vector<z3::expr>& e);
    PdrState(const std::vector<z3::expr>& e, std::shared_ptr<PdrState> s);
    // move constructors
    PdrState(std::vector<z3::expr>&& e);
    PdrState(std::vector<z3::expr>&& e, std::shared_ptr<PdrState> s);

    unsigned show(TextTable& table) const;

    unsigned no_marked() const;
  };

  struct Obligation
  {
    unsigned level;
    std::shared_ptr<PdrState> state;
    unsigned depth;

    Obligation(unsigned k, std::vector<z3::expr>&& cube, unsigned d);

    Obligation(unsigned k, const std::shared_ptr<PdrState>& s, unsigned d);

    // bool operator<(const Obligation& o) const { return this->level <
    // o.level; }
    bool operator<(const Obligation& o) const;
  };
} // namespace pdr
#endif // PDR_OBL
