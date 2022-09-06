#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "pdr-model.h"
#include "tactic.h"
#include <cstdint>
#include <z3++.h>

namespace pdr
{
  class Context
  {
   private:
    z3::context ctx;

   public:
    const bool delta;
    uint32_t seed;
    Tactic type;
    Context(z3::config& settings, bool d, bool random_seed);
    Context(z3::config& settings, bool d, unsigned seed);

    operator z3::context&();
    operator const z3::context&() const;

    z3::context& operator()() const;
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
