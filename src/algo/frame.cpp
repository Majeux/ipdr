#include "frame.h"
#include "logger.h"
#include "solver.h"
#include "z3-ext.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fmt/core.h>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>
#include <z3++.h>

namespace pdr
{
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  Frame::Frame(unsigned i) : level(i) {}

  void Frame::clear()
  {
    blocked_cubes.clear();
  }

  bool Frame::is_subsumed(vector<expr> const& new_cube) const
  {
    for (vector<expr> const& blocked_cube : blocked_cubes)
    {
      if (z3ext::subsumes_le(blocked_cube, new_cube))
      {
        return true; // equal or stronger clause found
      }
    }
    return false;
  }

  unsigned Frame::remove_subsumed(const vector<expr>& cube, bool remove_equal)
  {
    unsigned before = blocked_cubes.size();

    auto subsumes = [remove_equal](const vector<expr>& l, const vector<expr>& r)
    {
      return remove_equal ? z3ext::subsumes_le(l, r) : z3ext::subsumes_l(l, r);
    };

    for (auto it = blocked_cubes.begin(); it != blocked_cubes.end();)
    {
      if (subsumes(cube, *it))
        it = blocked_cubes.erase(it);
      else
        it++;
    }
    return before - blocked_cubes.size();
  }

  // interface
  //
  // cube is sorted by id()
  // block cube unless it, or a stronger version, is already blocked
  // TODO redundant, make void or make useful
  bool Frame::block(vector<expr> const& cube)
  {
    return blocked_cubes.insert(cube).second;
  }

  // assumes vectors in 'blocked_cubes' are sorted
  bool Frame::equals(const Frame& f) const
  {
    if (this->blocked_cubes.size() != f.blocked_cubes.size())
      return false;

    auto l_cube = this->blocked_cubes.begin();
    auto r_cube = f.blocked_cubes.begin();
    auto l_end  = this->blocked_cubes.end();
    auto r_end  = f.blocked_cubes.end();
    for (; l_cube != l_end && r_cube != r_end; l_cube++, r_cube++)
    {
      // if l_cubes* != r_cubes* -> return false
      if (l_cube->size() != r_cube->size())
        return false;

      auto l_lit = l_cube->begin();
      auto r_lit = r_cube->begin();
      for (; l_lit != l_cube->end() && r_lit != r_cube->end(); l_lit++, r_lit++)
        if ((*l_lit).id() != (*r_lit).id())
          return false;
    }
    return true;
  }

  std::vector<vector<expr>> Frame::diff(const Frame& f) const
  {
    vector<vector<expr>> out;
    std::set_difference(blocked_cubes.begin(), blocked_cubes.end(),
        f.blocked_cubes.begin(), f.blocked_cubes.end(), std::back_inserter(out),
        z3ext::std_expr_vector_less());
    return out;
  }

  const z3ext::CubeSet& Frame::get() const { return blocked_cubes; }
  bool Frame::empty() const { return blocked_cubes.size() == 0; }

  std::string Frame::blocked_str() const
  {
    std::string str(fmt::format("blocked cubes in frame {}\n", level));
    for (vector<expr> const& e : blocked_cubes)
      str += fmt::format("- {}\n", z3ext::join_ev(e, " & "));

    return str;
  }

} // namespace pdr
