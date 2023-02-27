#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "pdr-model.h"
#include "tactic.h"

#include <cstdint>
#include <z3++.h>

#define MIC_RETRIES_DEFAULT 3

namespace pdr
{
  class Context
  {
   public:
    z3::context& z3_ctx;
    uint32_t seed;
    Tactic type;
    uint32_t mic_retries;

    Context(z3::context& c, bool random_seed);
    Context(z3::context& c, unsigned seed);

    operator z3::context&();
    operator const z3::context&() const;

    z3::context& operator()();
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
