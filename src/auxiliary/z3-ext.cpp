#include "z3-ext.h"
#include "expr.h"

#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <z3++.h>

namespace z3ext
{
  using fmt::format;
  using std::optional;
  using std::string;
  using std::string_view;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  // LitStr Members
  //
  LitStr::LitStr(string_view a, bool s) : atom(a), sign(s) {}
  LitStr::LitStr(expr const& l)
  {
    const auto invalid = std::invalid_argument(
        format("\"{}\" is not boolean literal", l.to_string()));

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

  // ConstrainedCube Members
  //
  namespace constrained_cube
  {
    string constraint_str(size_t size)
    {
      return format("{}{}{}", prefix, size, suffix);
    }

    optional<size_t> constraint_size(string_view str)
    {
      // str must at least consist of tag and 1 character
      if (str.size() <= tag_length || mysat::primed::is_reserved_lit(str))
        return {};

      if (str.substr(0, prefix.length()) != prefix)
        return {};

      if (str.substr(str.length() - suffix.length(), suffix.length()) != suffix)
        return {};

      string_view order = str.substr(
          prefix.length(), str.length() - suffix.length() - prefix.length());

      return std::stoul(string(order));
    }

    optional<size_t> constraint_size(expr const& e)
    {
      return constraint_size(e.to_string());
    }

    bool stronger_constraint(expr const& a, expr const& b)
    {
      optional<size_t> a_con = constraint_size(a);
      optional<size_t> b_con = constraint_size(b);

      if (!a_con) // no constraint is always stronger or equal to any or none
        return true;
      // a_con
      if (!b_con) // idem: b is stronger
        return false;
      // a_con && b_con
      return *a_con >= *b_con; // predefined: higher size is stronger
    }

    std::vector<z3::expr> mk_constrained_cube(
        std::map<unsigned, size_t> const& order,
        std::vector<z3::expr> const& lits,
        size_t size)
    {
      vector<expr> rv(lits);
      return mk_constrained_cube(order, std::move(rv), size);
    }

    std::vector<z3::expr> mk_constrained_cube(
        std::map<unsigned, size_t> const& order,
        vector<expr>&& lits,
        size_t size)
    {
      assert(lits_ordered(lits));
      assert(   // at most one clit
          [&]() // lambda defined and immediately called
          {
            bool one_found{ false };
            for (expr const& e : lits)
              if (order.find(e) != order.end())
              {
                if (one_found)
                  return false;
                one_found = true;
              }
            return true;
          }());

      z3::context& ctx = lits.at(0).ctx();
      expr constraint  = ctx.bool_const(constraint_str(size).c_str());

      auto start = lits.begin();
      auto end   = lits.end();
      // see if a clit from order is present
      for (auto it = start; it != end; it++)
      {
        auto lit = order.find(it->id());
        if (lit != order.end()) // clit found
        {
          if (size >= lit->second) // new constraint subsumes old
            lits.erase(it);        // replace old
          break;
        }
      }

      // insert constraint and maintain sorted order
      auto position = std::lower_bound(start, end, constraint, cube_orderer);
      if (position != end)
        lits.insert(position, constraint);
      else
        lits.push_back(constraint); // inserting at end segfaults on ctx

      return std::move(lits);
    }

    bool subsumes_le(vector<expr> const& a, vector<expr> const& b)
    {
      // preliminary
      optional<size_t> a_con = constraint_size(a.back());
      optional<size_t> b_con = constraint_size(b.back());

      auto a_last = a_con ? std::prev(a.cend()) : a.cend(); // strip constraint
      auto b_last = b_con ? std::prev(b.cend()) : b.cend();

      auto cube_size = [](vector<expr> const& ev, bool constrained)
      { return ev.size() - constrained; };

      auto alits_subsume_blits = [&]() -> bool
      {
        // cube a is larger, so cannot subsume b
        if (cube_size(a, a_con.has_value()) > cube_size(b, b_con.has_value()))
          return false;

        return std::includes(
            b.cbegin(), b_last, a.cbegin(), a_last, cube_orderer);
      };

      // main body
      if (!a_con) // no constraint is always stronger or equal
        return alits_subsume_blits(); // return if a's literals are stronger

      if (b_con && *a_con >= *b_con)  // a has a stronger constraint
        return alits_subsume_blits(); // return if a's literals are stronger

      // b_con is stronger than a_con, or b is unconstrained
      return false; // so b is stronger even if cubes subsumes
    }

    bool cexpr_less::operator()(expr const& a, expr const& b) const
    {
      optional<size_t> a_con = constraint_size(a);
      optional<size_t> b_con = constraint_size(b);

      if (a_con && b_con)
        return *a_con < b_con;
      else if (!a_con && !b_con)
        return a.id() < b.id();
      else
        return !a_con && b_con; // any literal is earlier than a constraint
    }
  } // namespace constrained_cube

  // atoms and lits
  //
  expr minus(expr const& e) { return e.is_not() ? e.arg(0) : !e; }

  bool is_lit(expr const& e)
  {
    return e.is_bool() && (e.is_const() || (e.is_not() && e.arg(0).is_const()));
  }

  bool is_clause(expr const& e)
  {
    if (is_lit(e))
      return true;

    if (!e.is_or())
      return false;

    for (expr const& e : e.args())
      if (!is_lit(e))
        return false;

    return true;
  }

  bool are_clauses(expr_vector const& ev)
  {
    for (expr const& e : ev)
      if (!is_clause(e))
        return false;
    return true;
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

  vector<expr> order_lits_std(z3::expr_vector const& cube)
  {
    if (cube.size() == 0)
      return {};
    vector<expr> std_vec = convert(cube);
    order_lits(std_vec);
    return std_vec;
  }

  bool lits_ordered(vector<expr> const& cube)
  {
    return std::is_sorted(cube.cbegin(), cube.cend(), cube_orderer);
  }

  bool subsumes_l(expr_vector const& l, expr_vector const& r)
  {
    if (l.size() >= r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), cube_orderer);
  }

  bool subsumes_l(vector<expr> const& l, vector<expr> const& r)
  {
    if (l.size() >= r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), cube_orderer);
  }

  bool subsumes_le(expr_vector const& l, expr_vector const& r)
  {
    if (l.size() > r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), cube_orderer);
  }

  bool subsumes_le(vector<expr> const& l, vector<expr> const& r)
  {
    if (l.size() > r.size())
      return false;

    return std::includes(r.begin(), r.end(), l.begin(), l.end(), cube_orderer);
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

  bool std_expr_vector_less::operator()(
      const vector<expr>& l, const vector<expr>& r) const
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
    Witness::Witness(vector<expr> const& c, vector<expr> const& n)
        : curr(c), next(n)
    {
    }

    Witness::Witness(z3::expr_vector const& c, z3::expr_vector const& n)
    {
      for (expr const& e : c)
        curr.push_back(e);

      for (expr const& e : n)
        next.push_back(e);
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
    expr_vector to_cnf_vec(z3::expr e)
    {
      e = e.simplify();
      z3::tactic simplify(e.ctx(), "simplify");
      // z3::tactic simplify(e.ctx(), "ctx-simplify");
      z3::tactic cnf(e.ctx(), "tseitin-cnf");

      z3::tactic t = cnf & simplify;
      z3::goal g(e.ctx());

      g.add(e);
      z3::apply_result r = t(g);
      assert(r.size() == 1);
      z3::expr_vector clauses(e.ctx());
      for (size_t i = 0; i < r[0].size(); i++)
        clauses.push_back(r[0][i]);

      return clauses;
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
