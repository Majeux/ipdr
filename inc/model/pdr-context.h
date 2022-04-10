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
    uint32_t seed;
    Tactic type;
    Context(PebblingModel& m, bool d, bool random_seed);
    Context(PebblingModel& m, bool d, unsigned seed);
    z3::context& operator()() const;    
    PebblingModel& model();
    const PebblingModel& model() const;

   private:
    PebblingModel& _model;
  }; // class PDRcontext

} // namespace pdr
#endif // PDRCONTEXT_H

