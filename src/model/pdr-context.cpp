#include "pdr-context.h"

namespace pdr
{
  context::context(PDRModel& m, bool d, bool random_seed)
      : delta(d), seed(random_seed ? time(0) : 0u), _model(m)
  {
  }

  z3::context& context::operator()() const { return _model.ctx; }
  PDRModel& context:: model() const { return _model; }
  const PDRModel& context::const_model() const { return _model; }
} // namespace pdr
