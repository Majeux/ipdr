#include "z3pdr.h"
#include "logger.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include <z3++.h>

namespace pdr::test
{
  z3PDR::z3PDR(Context& c, Logger& l) : ctx(c), log(l), engine(ctx)
  {
    {
      z3::params p(ctx);
      p.set("engine", "spacer"); // z3 pdr implementation
      engine.set(p);
    }
  }
} // namespace pdr::test
