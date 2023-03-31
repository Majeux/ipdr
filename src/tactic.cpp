#include "tactic.h"

#include <fmt/core.h>
#include <stdexcept>

namespace pdr::tactic
{
  Tactic mk_tactic(std::string_view s)
  {
    if (s == constrain_str)
      return Tactic::constrain;
    if (s == relax_str)
      return Tactic::relax;
    if (s == binary_search_str)
      return Tactic::binary_search;
    if (s == inc_jump_str)
      return Tactic::inc_jump_test;
    if (s == inc_one_str)
      return Tactic::inc_one_test;

    throw std::invalid_argument(
        fmt::format("\"{}\" is not a valid pdr::Tactic", s));
  }
  std::string to_string(pdr::Tactic r)
  {
    switch (r)
    {
      case Tactic::basic: return "basic";
      case Tactic::constrain: return constrain_str;
      case Tactic::relax: return relax_str;
      case Tactic::binary_search: return binary_search_str;
      case Tactic::inc_jump_test: return inc_jump_str;
      case Tactic::inc_one_test: return inc_one_str;
      case Tactic::undef: return "???";
      default: throw std::invalid_argument("pdr::Tactic is undefined");
    }
  }
} // namespace pdr::tactic
