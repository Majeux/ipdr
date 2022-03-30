#include "pdr-context.h"

namespace pdr
{
  context::context(PebblingModel& m, bool d, bool random_seed)
      : delta(d), seed(random_seed ? time(0) : 0u), type(Tactic::undef), _model(m)
  {
  }

  z3::context& context::operator()() const { return _model.ctx; }
  PebblingModel& context:: model() const { return _model; }
  const PebblingModel& context::const_model() const { return _model; }
} // namespace pdr
