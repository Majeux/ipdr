#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "pdr-model.h"
#include "tactic.h"

#include <cstdint>
#include <z3++.h>

#define MIC_RETRIES_DEFAULT 3

namespace pdr
{
  // class containing algorithm settings
  // only consists of references or simple types
  // cheap to copy
  class Context
  {
   public:
    z3::context& z3_ctx;
    uint32_t seed;
    Tactic type;
    // in PDR::MIC if mic fails to reduce a clause this many times, consider the
    // current clause sufficient
    uint32_t mic_retries;

    Context(z3::context& c, bool random_seed);
    Context(z3::context& c, unsigned seed);

    operator z3::context&();
    operator const z3::context&() const;

    z3::context& operator()();
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
