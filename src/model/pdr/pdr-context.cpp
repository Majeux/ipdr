#include "pdr-context.h"

#include <iostream>
#include <memory>

namespace pdr
{
  namespace
  {
    void prepare(z3::context& ctx)
    {
      ctx.set("unsat_core", true);
      ctx.set("model", true);
    }

  } // namespace

  Context::Context(z3::context& c, unsigned s)
      : z3_ctx(c),
        seed(s),
        type(Tactic::undef),
        mic_retries(MIC_RETRIES_DEFAULT)
  {
    prepare(z3_ctx);
    std::cout << "z3 random seed: " << seed << std::endl;
  }

  Context::Context(z3::context& c, bool random_seed)
      : z3_ctx(c), type(Tactic::undef), mic_retries(MIC_RETRIES_DEFAULT)
  {
    prepare(z3_ctx);
    if (random_seed)
    {
      srand(time(0));
      seed = rand();
    }
    else seed = 0u;

    std::cout << "z3 random seed: " << seed << std::endl;
  }

  Context::operator z3::context&() { return z3_ctx; }
  Context::operator const z3::context&() const { return z3_ctx; }

  z3::context& Context::operator()() { return z3_ctx; }
} // namespace pdr
