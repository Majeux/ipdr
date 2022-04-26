#include "pdr-context.h"

namespace pdr
{
#warning add user conversions for z3::context&, PebblingModel& and const PebblingModel&
  Context::Context(pebbling::Model& m, bool d, unsigned seed)
      : delta(d), seed(seed), type(Tactic::undef), _model(m)
  {
    std::cout << "random seed " << seed << std::endl;
  }
  Context::Context(pebbling::Model& m, bool d, bool random_seed)
      : delta(d), type(Tactic::undef),
        _model(m)
  {
    srand(time(0));
    seed = random_seed ? rand() : 0u;
    std::cout << "random seed " << seed << std::endl;
  }

  Context::operator z3::context&(){ return _model.ctx; }
  Context::operator pebbling::Model&(){ return _model; }
  Context::operator const z3::context&() const{ return _model.ctx; }
  Context::operator const pebbling::Model&() const{ return _model; }

  z3::context& Context::operator()() const { return _model.ctx; }
  pebbling::Model& Context::model() { return _model; }
  const pebbling::Model& Context::model() const { return _model; }
} // namespace pdr
