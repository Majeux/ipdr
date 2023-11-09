#include "obligation.h"
#include "result.h"
#include "z3-ext.h"
#include <algorithm>

namespace pdr
{
  using std::shared_ptr;
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using TraceState = PdrResult::Trace::TraceState;

  // STATE MEMBERS
  //
  PdrState::PdrState(const vector<expr>& e)
      : cube(e), prev(shared_ptr<PdrState>())
  {
  }
  PdrState::PdrState(const vector<expr>& e, shared_ptr<PdrState> s)
      : cube(e), prev(s)
  {
  }
  // move constructors
  PdrState::PdrState(vector<expr>&& e)
      : cube(std::move(e)), prev(shared_ptr<PdrState>())
  {
  }
  PdrState::PdrState(vector<expr>&& e, shared_ptr<PdrState> s)
      : cube(std::move(e)), prev(s)
  {
  }

  // OBLIGATION MEMBERS
  //
  Obligation::Obligation(unsigned k, vector<expr>&& cube, unsigned d)
      : level(k), state(std::make_shared<PdrState>(std::move(cube))), depth(d)
  {
  }

  Obligation::Obligation(unsigned k, const shared_ptr<PdrState>& s, unsigned d)
      : level(k), state(s), depth(d)
  {
  }

  // bool operator<(const Obligation& o) const { return this->level <
  // o.level; }
  bool Obligation::operator<(const Obligation& o) const
  {
    if (this->level < o.level)
      return true;
    if (this->level > o.level)
      return false;
    if (this->depth < o.depth)
      return true;
    if (this->depth > o.depth)
      return false;

    return z3ext::std_expr_vector_less()(this->state->cube, o.state->cube);
  }
} // namespace pdr
