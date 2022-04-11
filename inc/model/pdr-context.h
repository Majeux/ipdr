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
    Context(PebblingModel& m, bool d, bool random_seed);
    Context(PebblingModel& m, bool d, unsigned seed);

	operator z3::context&();
	operator const z3::context&() const;
	operator PebblingModel&();
	operator const PebblingModel&() const;
    
	z3::context& operator()() const;
    PebblingModel& model();
    const PebblingModel& model() const;

   private:
    PebblingModel& _model;
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
