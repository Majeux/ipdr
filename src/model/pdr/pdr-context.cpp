#include "pdr-context.h"

namespace pdr
{
  Context::Context(z3::config& settings, bool d, unsigned seed)
      : ctx(settings), delta(d), seed(seed), type(Tactic::undef)
  {
  }
  Context::Context(z3::config& settings, bool d, bool random_seed)
      : ctx(settings), delta(d), type(Tactic::undef)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  Context::operator z3::context&() { return ctx; }
  Context::operator const z3::context&() const { return ctx; }
} // namespace pdr
