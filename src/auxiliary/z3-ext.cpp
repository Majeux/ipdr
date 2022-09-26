#include "z3-ext.h"

#include <algorithm>
#include <sstream>
#include <vector>
#include <z3++.h>

namespace z3ext
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  expr minus(const expr& e) { return e.is_not() ? e.arg(0) : !e; }

  bool is_lit(const expr& e)
  {
    return e.is_bool() && (e.is_not() || e.is_const());
  }

  expr_vector copy(const expr_vector& v)
  {
    z3::expr_vector new_v(v.ctx());
    for (const z3::expr& e : v)
      new_v.push_back(e);

    return new_v;
  }

  expr_vector negate(const expr_vector& lits)
  {
    expr_vector negated(lits.ctx());
    for (const expr& e : lits)
      negated.push_back(minus(e));
    return negated;
  }

  expr_vector negate(const vector<expr>& lits)
  {
    expr_vector negated(lits[0].ctx());
    for (const expr& e : lits)
      negated.push_back(minus(e));
    return negated;
  }

  expr_vector convert(const vector<expr>& vec)
  {
    assert(vec.size() > 0);
    expr_vector converted(vec[0].ctx());
    for (const expr& e : vec)
      converted.push_back(std::move(e));
    return converted;
  }

  expr_vector convert(vector<expr>&& vec)
  {
    assert(vec.size() > 0);
    expr_vector converted(vec[0].ctx());
    for (const expr& e : vec)
      converted.push_back(std::move(e));
    return converted;
  }

  vector<expr> convert(const expr_vector& vec)
  {
    vector<expr> converted;
    converted.reserve(vec.size());
    for (const expr& e : vec)
      converted.push_back(e);
    return converted;
  }

  expr_vector args(const expr& e)
  {
    expr_vector vec(e.ctx());
    for (unsigned i = 0; i < e.num_args(); i++)
      vec.push_back(e.arg(i));
    return vec;
  }

  void sort(expr_vector& v)
  {
    vector<expr> std_vec = convert(v);
    std::sort(std_vec.begin(), std_vec.end(), expr_less());
    v = convert(std::move(std_vec));
  }

  void sort_cube(expr_vector& v)
  {
    vector<expr> std_vec = convert(v);
    std::sort(std_vec.begin(), std_vec.end(), lit_less());
    v = convert(std::move(std_vec));
  }

  bool subsumes_l(const expr_vector& l, const expr_vector& r)
  {
    if (l.size() >= r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), expr_less());
  }

  bool subsumes_le(const expr_vector& l, const expr_vector& r)
  {
    if (l.size() > r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), expr_less());
  }

  // COMPARATOR FUNCTORS
  //
  bool lit_less::operator()(const z3::expr& l, const z3::expr& r) const
  {
    unsigned a = l.is_not() ? l.arg(0).id() : l.id();
    unsigned b = r.is_not() ? r.arg(0).id() : r.id();

    return a < b;
  };

  bool expr_less::operator()(const z3::expr& l, const z3::expr& r) const
  {
    return l.id() < r.id();
  };

  size_t expr_hash::operator()(const z3::expr& l) const
  {
    return l.id();
  };


  bool expr_vector_less::operator()(
      const expr_vector& l, const expr_vector& r) const
  {
    auto l_it = l.begin();
    auto r_it = r.begin();

    for (; l_it != l.end() && r_it != r.end(); l_it++, r_it++)
    {
      if ((*l_it).id() < (*r_it).id())
        return true;
      if ((*l_it).id() > (*r_it).id())
        return false;
    }
    // all elements equal to a point
    return l.size() < r.size();
  }

  // SOLVER AIDS
  //
  namespace solver
  {
    expr_vector get_witness(const z3::solver& s)
    {
      return convert(get_std_witness(s));
    }

    std::vector<expr> get_std_witness(const z3::solver& s)
    {
      z3::model m      = s.get_model();

      std::vector<z3::expr> std_vec;
      std_vec.reserve(m.num_consts());

      for (unsigned i = 0; i < m.size(); i++)
      {
        z3::func_decl f        = m[i];
        z3::expr boolean_value = m.get_const_interp(f);
        z3::expr literal       = f();

        if (boolean_value.is_true())
          std_vec.push_back(literal);
        else if (boolean_value.is_false())
          std_vec.push_back(!literal);
        else
          throw std::runtime_error("model contains non-constant");
      }
      std::sort(std_vec.begin(), std_vec.end(), z3ext::expr_less());
      return std_vec;
    }

    std::optional<z3::expr_vector> check_witness(z3::solver& s)
    {
      z3::check_result r = s.check();
      assert(not(r == z3::check_result::unknown));
      if (r == z3::check_result::sat)
        return get_witness(s);
      else
        return {};
    }

    std::optional<z3::expr_vector> check_witness(
        z3::solver& s, const z3::expr_vector& assumptions)
    {
      z3::check_result r = s.check(assumptions);
      assert(not(r == z3::check_result::unknown));
      if (r == z3::check_result::sat)
        return get_witness(s);
      else
        return {};
    }
  } // namespace solver

  // TSEYTIN ENCODING
  //
  namespace tseytin
  {
    expr_vector to_cnf_vec(const z3::expr& e)
    {
      z3::tactic t1(e.ctx(), "simplify"), t2(e.ctx(), "tseitin-cnf");
      z3::tactic t = t1 & t2;
      z3::goal g(e.ctx());

      g.add(e);
      z3::apply_result r = t(g);
      assert(r.size() == 1);
      z3::expr_vector cnf(e.ctx());
      for (size_t i = 0; i < r[0].size(); i++)
        cnf.push_back(r[0][i]);

      return cnf;
    }

    expr to_cnf(const expr& e) { return z3::mk_and(to_cnf_vec(e)); }

    expr add_and(
        expr_vector& cnf, const string& name, const expr& a, const expr& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(c || !a || !b);
      cnf.push_back(!c || a);
      cnf.push_back(!c || b);
      return c;
    }

    expr add_or(
        expr_vector& cnf, const string& name, const expr& a, const expr& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(!c || a || b);
      cnf.push_back(c || !a);
      cnf.push_back(c || !b);
      return c;
    }

    expr add_implies(
        expr_vector& cnf, const string& name, const expr& a, const expr& b)
    {
      return add_or(cnf, name, !a, b);
    }

    expr add_xor(
        expr_vector& cnf, const string& name, const expr& a, const expr& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(!c || !a || !b);
      cnf.push_back(!c || a || b);
      cnf.push_back(c || a || !b);
      cnf.push_back(c || !a || b);
      return c;
    }

    expr add_xnor(
        expr_vector& cnf, const string& name, const expr& a, const expr& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(c || !a || !b);
      cnf.push_back(c || a || b);
      cnf.push_back(!c || a || !b);
      cnf.push_back(!c || !a || b);
      return c;
    }
  } // namespace tseytin
} // namespace z3ext
