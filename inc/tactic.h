#ifndef PDR_TACTIC_H
#define PDR_TACTIC_H

#include <string>

namespace pdr
{
  // clang-format off
  enum class Tactic
  {
    undef, 
    basic, increment, decrement, 
    inc_jump_test, inc_one_test,
  };
  // clang-format on

  namespace tactic
  {
    const std::string decrement_str("dec");
    const std::string increment_str("inc");
    const std::string inc_jump_str("inc-jump-test");
    const std::string inc_one_str("inc-one-test");

    std::string to_string(pdr::Tactic r);
  } // namespace tactic
} // namespace pdr

#endif // PDR_TACTIC_H
