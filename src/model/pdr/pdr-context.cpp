#include "pdr-context.h"

namespace pdr
{
  Context::Context(bool d, unsigned seed)
      : ctx(make_settings()), delta(d), seed(seed), type(Tactic::undef)
  {
  }
  Context::Context(bool d, bool random_seed)
      : ctx(make_settings()), delta(d), type(Tactic::undef)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  z3::config& Context::make_settings()
  {
    settings.set("unsat_core", true);
    settings.set("model", true);
    return settings;
  }

  Context::operator z3::context&() { return ctx; }
  Context::operator const z3::context&() const { return ctx; }

  z3::context& Context::operator()() { return ctx; }
} // namespace pdr
