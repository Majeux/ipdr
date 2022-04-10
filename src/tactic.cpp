#include "tactic.h"

#include <stdexcept>

namespace pdr::tactic
{
  std::string to_string(pdr::Tactic r)
  {
    switch (r)
    {
      case pdr::Tactic::basic: return "basic";
      case pdr::Tactic::decrement: return decrement_str;
      case pdr::Tactic::increment: return increment_str;
      case pdr::Tactic::inc_jump_test: return inc_jump_str;
      case pdr::Tactic::inc_one_test: return inc_one_str;
      default: throw std::invalid_argument("pdr::Tactic is undefined");
    }
  }
} // namespace pdr

