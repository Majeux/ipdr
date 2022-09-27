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
     IModel& model;

   public:
    const bool delta;
    uint32_t seed;
    Tactic type;
    Context(IModel& c, bool d, bool random_seed);
    Context(IModel& c, bool d, unsigned seed);

    operator z3::context&();
    operator const z3::context&() const;

    z3::context& operator()();
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
