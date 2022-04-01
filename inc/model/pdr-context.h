#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "pdr-model.h"
#include <cstdint>
#include <z3++.h>

namespace pdr
{
  enum class Tactic
  {
    undef, 
    basic, increment, decrement, 
    inc_jump_test, inc_one_test,
  };

  class Context
  {
   public:
    const bool delta;
    const uint32_t seed;
    Tactic type; // algorithm may switch between runs types

    Context(PebblingModel& m, bool d, bool random_seed);
    z3::context& operator()() const;    
    PebblingModel& model() const;
    const PebblingModel& const_model() const;

   private:
    PebblingModel& _model;
  }; // class PDRcontext

} // namespace pdr
#endif // PDRCONTEXT_H

