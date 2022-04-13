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
   public:
    const bool delta;
    uint32_t seed;
    Tactic type;
    Context(pebbling::Model& m, bool d, bool random_seed);
    Context(pebbling::Model& m, bool d, unsigned seed);

    operator z3::context&();
    operator const z3::context&() const;
    operator pebbling::Model&();
    operator const pebbling::Model&() const;

    z3::context& operator()() const;
    pebbling::Model& model();
    const pebbling::Model& model() const;

   private:
    pebbling::Model& _model;
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
