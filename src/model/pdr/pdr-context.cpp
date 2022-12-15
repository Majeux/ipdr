#include "pdr-context.h"

#include <iostream>

namespace pdr
{
  Context::Context(IModel& m, unsigned s) : ts(m), seed(s), type(Tactic::undef)
  {
    std::cout << "random seed " << seed << std::endl;
  }

  Context::Context(IModel& m, bool random_seed) : ts(m), type(Tactic::undef)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  Context::operator z3::context&() { return ts.ctx; }
  Context::operator const z3::context&() const { return ts.ctx; }

  z3::context& Context::operator()() { return ts.ctx; }
} // namespace pdr
