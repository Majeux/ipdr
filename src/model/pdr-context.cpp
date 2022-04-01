#include "pdr-context.h"

namespace pdr
{
  Context::Context(PebblingModel& m, bool d, bool random_seed)
      : delta(d), seed(random_seed ? time(0) : 0u), type(Tactic::undef), _model(m)
  {
  }

  z3::context& Context::operator()() const { return _model.ctx; }
  PebblingModel& Context:: model() const { return _model; }
  const PebblingModel& Context::const_model() const { return _model; }
} // namespace pdr
