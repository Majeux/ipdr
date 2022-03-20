#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "pdr-model.h"
#include <cstdint>
#include <z3++.h>

namespace pdr
{
  enum class Tactic
  {
    undef, basic, increment, decrement
  };

  class context
  {
   public:
    const bool delta;
    const uint32_t seed;
    Tactic type; // algorithm may switch between runs types

    context(Model& m, bool d, bool random_seed);
    z3::context& operator()() const;    
    Model& model() const;
    const Model& const_model() const;

   private:
    Model& _model;
  }; // class PDRcontext

} // namespace pdr
#endif // PDRCONTEXT_H

