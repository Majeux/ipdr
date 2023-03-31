#include "z3-ext.h"

#include <algorithm>
#include <fmt/core.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <z3++.h>

namespace z3ext
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  // atoms and lits
  //
  LitStr::LitStr(std::string_view a, bool s) : atom(a), sign(s) {}
  LitStr::LitStr(expr const& l)
  {
    const auto invalid = std::invalid_argument(
        fmt::format("\"{}\" is not boolean literal", l.to_string()));

    if (!l.is_bool())
      throw invalid;

    if (l.is_const())
    {
      atom = l.to_string();
      sign = true;
    }
    else if (l.is_not())
    {
      if (!l.arg(0).is_const())
        throw invalid;
      atom = l.arg(0).to_string();
      sign = false;
    }
    else
      throw invalid;
  }

  z3::expr LitStr::to_expr(z3::context& ctx)
  {
    if (sign)
      return ctx.bool_const(atom.c_str());

    return !ctx.bool_const(atom.c_str());
  }
  
  std::string LitStr::to_string() const
  {
    return fmt::format("{} -> {}", atom, sign ? "true" : "false");
  }


  expr minus(expr const& e) { return e.is_not() ? e.arg(0) : !e; }

  bool is_lit(expr const& e)
  {
    return e.is_bool() && (e.is_not() || e.is_const());
  }

  expr strip_not(expr const& e)
  {
    expr rv = e;
    if (e.is_not())
      rv = e.arg(0);

    if (!rv.is_const())
    {
      const std::string msg =
          fmt::format("`e` is not a literal:\n {}", e.to_string());
      throw std::invalid_argument(msg);
    }

    assert(rv.is_const());
    return rv;
  }

  expr_vector mk_expr_vec(std::initializer_list<z3::expr> l)
  {
    assert(not std::empty(l));
    expr_vector rv(l.begin()->ctx());
    for (expr e : l)
      rv.push_back(e);

    return rv;
  }

  expr_vector copy(expr_vector const& v)
  {
    z3::expr_vector new_v(v.ctx());
    for (const z3::expr& e : v)
      new_v.push_back(e);

    return new_v;
  }

  expr_vector negate(expr_vector const& lits)
  {
    expr_vector negated(lits.ctx());
    for (expr const& e : lits)
      negated.push_back(minus(e));
    return negated;
  }

  expr_vector negate(vector<expr> const& lits)
  {
    expr_vector negated(lits[0].ctx());
    for (expr const& e : lits)
      negated.push_back(minus(e));
    return negated;
  }

  expr_vector convert(vector<expr> const& vec)
  {
    assert(vec.size() > 0);
    expr_vector converted(vec[0].ctx());
    for (expr const& e : vec)
      converted.push_back(std::move(e));
    return converted;
  }

  expr_vector convert(vector<expr>&& vec)
  {
    assert(vec.size() > 0);
    expr_vector converted(vec[0].ctx());
    for (expr const& e : vec)
      converted.push_back(std::move(e));
    return converted;
  }

  vector<expr> convert(expr_vector const& vec)
  {
    vector<expr> converted;
    converted.reserve(vec.size());
    for (expr const& e : vec)
      converted.push_back(e);
    return converted;
  }

  expr_vector args(expr const& e)
  {
    expr_vector vec(e.ctx());
    for (unsigned i = 0; i < e.num_args(); i++)
      vec.push_back(e.arg(i));
    return vec;
  }

  void sort_lits(vector<expr>& cube)
  {
    if (cube.size() == 0)
      return;
    std::sort(cube.begin(), cube.end(), lit_less());
  }

  void sort_lits(expr_vector& cube)
  {
    if (cube.size() == 0)
      return;
    vector<expr> std_vec = convert(cube);
    sort_lits(std_vec);
    cube = convert(std::move(std_vec));
  }

  void sort_exprs(std::vector<z3::expr>& v)
  {
    std::sort(v.begin(), v.end(), expr_less());
  }

  void sort_exprs(expr_vector& v)
  {
    vector<expr> std_vec = convert(v);
    sort_exprs(std_vec);
    v = convert(std::move(std_vec));
  }

  void order_lits(std::vector<z3::expr>& cube)
  {
    std::sort(cube.begin(), cube.end(), cube_orderer);
  }

  void order_lits(z3::expr_vector& cube)
  {
    if (cube.size() == 0)
      return;
    vector<expr> std_vec = convert(cube);
    order_lits(std_vec);
    cube = convert(std::move(std_vec));
  }

  bool lits_ordered(vector<expr> const& cube)
  {
    return std::is_sorted(cube.cbegin(), cube.cend(), cube_orderer);
  }

  bool subsumes_l(expr_vector const& l, expr_vector const& r)
  {
    if (l.size() >= r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), expr_less());
  }

  bool subsumes_l(vector<expr> const& l, expr_vector const& r)
  {
    if (l.size() >= r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), expr_less());
  }

  bool subsumes_le(expr_vector const& l, expr_vector const& r)
  {
    if (l.size() > r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), expr_less());
  }

  bool subsumes_le(vector<expr> const& l, expr_vector const& r)
  {
    if (l.size() > r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), expr_less());
  }

  bool eq(expr_vector const& l, expr_vector const& r)
  {
    if (l.size() != r.size())
      return false;

    expr_vector::iterator l_it = l.begin(), r_it = r.begin();

    for (; l_it != l.end() && r_it != r.end(); l_it++, r_it++)
    {
      if ((*l_it).id() != (*r_it).id())
        return false;
    }

    return true;
  }

  bool quick_implies(z3::expr_vector const& l, z3::expr_vector const& r)
  {
    z3::solver s(l.ctx());
    s.add(z3::implies(z3::mk_and(l), z3::mk_and(r)));
    return s.check() == z3::check_result::sat;
  }
  // COMPARATOR FUNCTORS
  //

  // LESS THAN
  bool strip_not_less(expr const& l, expr const& r)
  {
    unsigned a = l.is_not() ? l.arg(0).id() : l.id();
    unsigned b = r.is_not() ? r.arg(0).id() : r.id();

    return a < b;
  }

  bool id_less(expr const& a, expr const& b) { return a.id() < b.id(); }

  bool lit_less::operator()(expr const& l, expr const& r) const
  {
    return strip_not_less(l, r);
  };

  bool expr_less::operator()(expr const& l, expr const& r) const
  {
    return id_less(l, r);
  };

  // bool cube_order_less::operator()(const expr& l, const expr& r) const
  // {
  //   return id_less(l, r);
  // };

  bool expr_vector_less::operator()(
      expr_vector const& l, expr_vector const& r) const
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

  // END LESS THAN

  size_t expr_hash::operator()(expr const& l) const { return l.id(); };

  // SOLVER AIDS
  //
  namespace solver
  {
    Witness::Witness(z3::expr_vector const& c, z3::expr_vector const& n)
        : curr(c), next(n)
    {
    }

    expr_vector get_witness(z3::solver const& s)
    {
      return convert(get_std_witness(s));
    }

    std::vector<expr> get_std_witness(z3::solver const& s)
    {
      z3::model m = s.get_model();

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

      order_lits(std_vec);

      return std_vec;
    }

    expr_vector get_core(z3::solver const& s)
    {
      return convert(get_std_core(s));
    }

    vector<expr> get_std_core(z3::solver const& s)
    {
      vector<expr> core = convert(s.unsat_core());
      order_lits(core);
      return core;
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
        z3::solver& s, z3::expr_vector const& assumptions)
    {
      z3::check_result r = s.check(assumptions);
      assert(not(r == z3::check_result::unknown));
      if (r == z3::check_result::sat)
        return get_witness(s);
      else
        return {};
    }
  } // namespace solver

  std::string expr_info(expr const& e, unsigned level = 0)
  {
    std::string indent(level, '\t');
    std::stringstream ss;

    ss << indent
       << fmt::format(
              "type: {} - args: {}", e.get_sort().to_string(), e.num_args());

    return ss.str();
  }

  namespace fixedpoint
  {
    std::vector<z3::expr> extract_trace_states(z3::fixedpoint& engine)
    {
      std::vector<z3::expr> rv;
      // answer {
      // arg(0) = proof {
      //  arg(0) = define-target
      //  arg(1) = recursive-state
      //  arg(2) = assert-target
      //  }
      // arg(1) = query
      // arg(2) = result
      // }
      expr recursive_state = engine.get_answer().arg(0).arg(1);

      // recursive-state {
      //  arg(0) = define-step
      //  arg(1) = step
      //  arg(2) = recursive-state
      //  arg(3) = destination state
      // }
      while (recursive_state.num_args() == 4)
      {
        rv.push_back(recursive_state.arg(3));

        recursive_state = recursive_state.arg(2);
      }
      assert(recursive_state.num_args() == 2);
      rv.push_back(recursive_state.arg(1));

      std::reverse(rv.begin(), rv.end());
      return rv;
    }
  } // namespace fixedpoint

  // TSEYTIN ENCODING
  //
  namespace tseytin
  {
    expr_vector to_cnf_vec(z3::expr const& e)
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

    expr to_cnf(expr const& e) { return z3::mk_and(to_cnf_vec(e)); }

    expr add_and(
        expr_vector& cnf, string const& name, expr const& a, expr const& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(c || !a || !b);
      cnf.push_back(!c || a);
      cnf.push_back(!c || b);
      return c;
    }

    expr add_or(
        expr_vector& cnf, string const& name, expr const& a, expr const& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(!c || a || b);
      cnf.push_back(c || !a);
      cnf.push_back(c || !b);
      return c;
    }

    expr add_implies(
        expr_vector& cnf, string const& name, expr const& a, expr const& b)
    {
      return add_or(cnf, name, !a, b);
    }

    expr add_xor(
        expr_vector& cnf, string const& name, expr const& a, expr const& b)
    {
      expr c = cnf.ctx().bool_const(name.c_str());
      cnf.push_back(!c || !a || !b);
      cnf.push_back(!c || a || b);
      cnf.push_back(c || a || !b);
      cnf.push_back(c || !a || b);
      return c;
    }

    expr add_xnor(
        expr_vector& cnf, string const& name, expr const& a, expr const& b)
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
