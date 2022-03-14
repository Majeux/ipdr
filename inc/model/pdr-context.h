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

    context(Model& m, bool d, bool r);
    z3::context& operator()() const;    
    Model& model() const;
    const Model& const_model() const;

   private:
    Model& _model;
  }; // class PDRcontext

} // namespace pdr
#endif // PDRCONTEXT_H

