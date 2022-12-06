#ifndef Z3PDR_H
#define Z3PDR_H

#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "result.h"

#include <z3++.h>

namespace pdr::test 
{
  class z3PDR
  {
    public:
    z3PDR(Context& m, Logger& l);

    PdrResult run();

    private:
    Context& ctx;
    Logger& log;
    z3::fixedpoint engine;
  };
} // namespace pdr::z3

#endif // Z3PDR_H
