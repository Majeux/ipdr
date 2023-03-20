#ifndef PDR_TACTIC_H
#define PDR_TACTIC_H

#include <string>

namespace pdr
{
  // clang-format off
  enum class Tactic
  {
    undef, 
    basic, relax, constrain, binary_search,
    inc_jump_test, inc_one_test,
  };
  // clang-format on

  namespace tactic
  {
    inline static const std::string constrain_str{"constrain"};
    inline static const std::string relax_str{"relax"};
    inline static const std::string binary_search_str{"binary-search"};
    inline static const std::string inc_jump_str{"inc-jump-test"};
    inline static const std::string inc_one_str{"inc-one-test"};

    Tactic mk_tactic(std::string_view s);
    std::string to_string(pdr::Tactic r);
  } // namespace tactic
} // namespace pdr

#endif // PDR_TACTIC_H
