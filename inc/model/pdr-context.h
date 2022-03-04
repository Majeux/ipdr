#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "pdr-model.h"
#include <cstdint>
#include <z3++.h>

namespace pdr
{
  class context
  {
   public:
    const bool delta;
    const uint32_t seed;

    context(PDRModel& m, bool d, bool r);
    z3::context& operator()() const;    
    PDRModel& model() const;
    const PDRModel& const_model() const;

   private:
    PDRModel& _model;
  }; // class PDRcontext

} // namespace pdr
#endif // PDRCONTEXT_H

