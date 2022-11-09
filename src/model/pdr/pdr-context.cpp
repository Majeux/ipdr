#include "pdr-context.h"

namespace pdr
{
  Context::Context(IModel& m, unsigned s) : model(m), seed(s), type(Tactic::undef) {}

  Context::Context(IModel& m, bool random_seed)
      : model(m), type(Tactic::undef)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  Context::operator z3::context&() { return model.ctx; }
  Context::operator const z3::context&() const { return model.ctx; }

  z3::context& Context::operator()() { return model.ctx; }
} // namespace pdr
