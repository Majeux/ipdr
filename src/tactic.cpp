#include "tactic.h"

#include <stdexcept>

namespace pdr::tactic
{
  std::string to_string(pdr::Tactic r)
  {
    switch (r)
    {
      case Tactic::basic: return "basic";
      case Tactic::constrain: return constrain_str;
      case Tactic::relax: return relax_str;
      case Tactic::inc_jump_test: return inc_jump_str;
      case Tactic::inc_one_test: return inc_one_str;
      case Tactic::undef: return "???";
       default: throw std::invalid_argument("pdr::Tactic is undefined");
    }
  }
} // namespace pdr

