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
    Context(pdr::IModel& m, bool d, bool random_seed);
    Context(pdr::IModel& m, bool d, unsigned seed);

    operator z3::context&();
    operator const z3::context&() const;
    operator pdr::IModel&();
    operator const pdr::IModel&() const;

    z3::context& operator()() const;
    pdr::IModel& model();
    const pdr::IModel& model() const;

   private:
    pdr::IModel& _model;
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
