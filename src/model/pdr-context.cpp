#include "pdr-context.h"

namespace pdr
{
#warning add user conversions for z3::context&, PebblingModel& and const PebblingModel&
  Context::Context(PebblingModel& m, bool d, unsigned seed)
      : delta(d), seed(seed), type(Tactic::undef), _model(m)
  {
    std::cout << "random seed " << seed << std::endl;
  }
  Context::Context(PebblingModel& m, bool d, bool random_seed)
      : delta(d), seed(random_seed ? time(0) : 0u), type(Tactic::undef),
        _model(m)
  {
    std::cout << "random seed " << seed << std::endl;
  }

  z3::context& Context::operator()() const { return _model.ctx; }
  PebblingModel& Context::model() { return _model; }
  const PebblingModel& Context::model() const { return _model; }
} // namespace pdr
