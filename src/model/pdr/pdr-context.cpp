#include "pdr-context.h"

#include <iostream>
#include <memory>

namespace pdr
{
  Context::Context(z3::context& c, unsigned s)
      : z3_ctx(c),
        seed(s),
        type(Tactic::undef),
        mic_retries(MIC_RETRIES_DEFAULT)
  {
    std::cout << "random seed " << seed << std::endl;
  }

  Context::Context(z3::context& c, bool random_seed)
      : z3_ctx(c), type(Tactic::undef), mic_retries(MIC_RETRIES_DEFAULT)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  Context::operator z3::context&() { return z3_ctx; }
  Context::operator const z3::context&() const { return z3_ctx; }

  z3::context& Context::operator()() { return z3_ctx; }
} // namespace pdr
