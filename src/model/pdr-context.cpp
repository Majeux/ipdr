#include "pdr-context.h"

namespace pdr
{
  context::context(Model& m, bool d, bool random_seed)
      : delta(d), seed(random_seed ? time(0) : 0u), type(Tactic::undef), _model(m)
  {
  }

  z3::context& context::operator()() const { return _model.ctx; }
  Model& context:: model() const { return _model; }
  const Model& context::const_model() const { return _model; }
} // namespace pdr
