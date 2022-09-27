#include "pdr-context.h"

namespace pdr
{
  Context::Context(IModel& m, bool d, unsigned s) : model(m), delta(d), seed(s), type(Tactic::undef) {}

  Context::Context(IModel& m, bool d, bool random_seed)
      : model(m), delta(d), type(Tactic::undef)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  Context::operator z3::context&() { return model.ctx; }
  Context::operator const z3::context&() const { return model.ctx; }

  z3::context& Context::operator()() { return model.ctx; }
} // namespace pdr
