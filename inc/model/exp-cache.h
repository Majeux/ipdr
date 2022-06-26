#ifndef EXP_CACHE
#define EXP_CACHE

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <z3++.h>

#include "string-ext.h"
#include "z3-ext.h"

namespace mysat::primed
{
  class Lit
  {
   public:
    Lit(z3::context& c, const std::string& name)
        : ctx(c), current(ctx), next(ctx)
    {
      std::string next_name = fmt::format("{}.p", name);
      current               = ctx.bool_const(name.c_str());
      next                  = ctx.bool_const(next_name.c_str());
    }

    operator const z3::expr&() { return current; }
    const z3::expr& operator()() { return current; }
    const z3::expr& p() { return next; }

    z3::expr unchanged() const { return current == next; }

   private:
    z3::context& ctx;

    z3::expr current;
    z3::expr next;
  }; // class Lit

  class BitVec
  {
   public:
    // name = base name for the vector, each bit is "name[0], name[1], ..."
    // max = the maximum (unsigned) integer value the vector should describe
    BitVec(z3::context& c, const std::string& name, size_t max)
        : size(std::log2(max) + 1), ctx(c), current(ctx), next(ctx)
    {
      assert(max < INT_MAX);
      for (size_t i = 0; i < size; i++)
      {
        using fmt::format;

        std::string curr_name = format("{}_{}", name, i);
        std::string next_name = format("{}_{}.p", name, i);
        current.push_back(ctx.bool_const(curr_name.c_str()));
        next.push_back(ctx.bool_const(next_name.c_str()));
      }
    }

    // return all literals comprising the vector
    operator const z3::expr_vector&() { return current; }
    const z3::expr_vector& operator()() { return current; }
    const z3::expr_vector& p() { return next; }

    // access individual literals
    z3::expr operator()(size_t i) { return current[i]; }
    z3::expr p(size_t i) { return next[i]; }

    // bitvector to unsigned integer conversions
    z3::expr_vector uint(unsigned n) const { return unint_to_lits(n, false); }
    z3::expr_vector uint_p(unsigned n) const { return unint_to_lits(n, true); }

    // operator to expression conversions
    z3::expr equals(unsigned n) const { return z3::mk_and(uint(n)); }
    z3::expr p_equals(unsigned n) const { return z3::mk_and(uint_p(n)); }

    z3::expr unchanged() const
    {
      z3::expr_vector conj(ctx);
      for (size_t i = 0; i < size; i++)
        conj.push_back(current[i] == next[i]);

      return z3::mk_and(conj);
    }

    z3::expr less(unsigned n) const
    {
      if (n == 0)
        return ctx.bool_val(false); // unsigned, so impossible

      std::bitset<sizeof(n)> bits(n);
      assert(size <= bits.size());

      z3::expr_vector pre_set(ctx), post_set(ctx);
      bool set = false;
      for (int i = bits.size() - 1; i >= 0; i--)
      {
        if (bits.test(i))
        {
          if (!set)
          {
            if (i > (int)size - 1) // n is larger than the bv can hold
              return ctx.bool_val(false);
          }
          set = true;
          post_set.push_back(!current[i]);
        }
        else if (!set && i <= (int)size - 1)
          pre_set.push_back(!current[i]);
      }

      z3::expr cube(ctx), clause(ctx);

      // simplify resulting expression
      if (pre_set.size() == 1)
        cube = pre_set[0];
      else
        cube = z3::mk_and(pre_set);

      if (post_set.size() == 1)
        clause = post_set[0];
      else
        clause = z3::mk_or(post_set);

      if (pre_set.size() == 0)
      {
        assert(post_set.size() > 0);
        return clause;
      }
      if (post_set.size() == 0)
      {
        assert(pre_set.size() > 0);
        return cube;
      }

      return cube & clause;
    }

   private:
    size_t size;
    z3::context& ctx;

    z3::expr_vector current;
    z3::expr_vector next;

    z3::expr_vector unint_to_lits(unsigned n, bool primed) const
    {
      z3::expr_vector rv(ctx), reversed(ctx);
      std::bitset<sizeof(n)> bits(n);
      assert(size <= bits.size());
      for (size_t i = 0; i < current.size(); i++)
      {
        if (n & 1) // first bit is 1
          reversed.push_back(primed ? next[i] : current[i]);
        else
          reversed.push_back(primed ? !next[i] : !current[i]);

        n >>= 1;
      }

      for (int i = reversed.size() - 1; i >= 0; i--)
        rv.push_back(reversed[i]);

      return rv;
    }
  };
} // namespace mysat::primed

class PrimedExpression
{
 public:
  PrimedExpression(z3::context& ctx) : current(ctx), next(ctx) {}

  operator const z3::expr&() { return current; }
  const z3::expr& operator()() { return current; }
  const z3::expr& p() { return next; }

  static PrimedExpression array(
      z3::context& ctx, std::string_view n, z3::sort element_type)
  {
    std::string name(n);
    std::string next_name = fmt::format("{}.p", name);
    z3::sort Array        = ctx.array_sort(ctx.int_sort(), element_type);

    z3::expr e  = ctx.constant(name.c_str(), Array);
    z3::expr ep = ctx.constant(next_name.c_str(), Array);

    return PrimedExpression(e, ep);
  }

 private:
  z3::expr current;
  z3::expr next;

  PrimedExpression(const z3::expr& e, const z3::expr& ep) : current(e), next(ep)
  {
  }
};

class PrimedExpressions
{
 public:
  PrimedExpressions(z3::context& c) : ctx(c), current(ctx), next(ctx) {}

  size_t size() const
  {
    assert(current.size() == next.size());
    return current.size();
  }

  operator const z3::expr_vector&() { return current; }
  const z3::expr_vector& operator()() { return current; }
  const z3::expr_vector& p() { return next; }

  z3::expr operator()(size_t i) const { return current[i]; }
  z3::expr p(size_t i) const { return next[i]; }

  PrimedExpressions& add_array(const std::string& name, z3::sort element_type)
  {
    assert(!finished);
    std::string next_name = fmt::format("{}.p", name);
    z3::sort Array        = ctx.array_sort(ctx.int_sort(), element_type);

    current.push_back(ctx.constant(name.c_str(), Array));
    next.push_back(ctx.constant(next_name.c_str(), Array));

    return *this;
  }

  PrimedExpressions& add_bitvec(const std::string& name, unsigned size)
  {
    assert(!finished);
    std::string next_name = fmt::format("{}.p", name);

    current.push_back(ctx.bv_const(name.c_str(), size));
    next.push_back(ctx.bv_const(next_name.c_str(), size));

    return *this;
  }

  PrimedExpressions& add_bool(const std::string& name)
  {
    assert(!finished);
    std::string next_name = fmt::format("{}.p", name);

    current.push_back(ctx.bool_const(name.c_str()));
    next.push_back(ctx.bool_const(next_name.c_str()));

    return *this;
  }

  PrimedExpressions& add_int(const std::string& name)
  {
    assert(!finished);
    std::string next_name = fmt::format("{}.p", name);

    current.push_back(ctx.int_const(name.c_str()));
    next.push_back(ctx.int_const(next_name.c_str()));

    return *this;
  }

  void finish() { finished = true; }

 private:
  bool finished{ false };
  z3::context& ctx;

  z3::expr_vector current;
  z3::expr_vector next;
};

class ExpressionCache
{
 private:
  // add no new expressions after finish() has been called
  bool finished = false;
  enum class Encoding
  {
    UNKNOWN,
    LITERALS,
    EXPRESSIONS
  } encodes;
  z3::context& ctx;
  std::unordered_map<unsigned, int> literal_index;
  std::unordered_map<unsigned, int> literal_index_p;

  z3::expr_vector current;
  z3::expr_vector next;

 public:
  ExpressionCache(z3::context& c)
      : encodes(Encoding::UNKNOWN), ctx(c), current(c), next(c)
  {
  }

  int indexof(const z3::expr& e) const { return literal_index.at(e.id()); }
  // checks if e is an atom in current. fails if e is not an atom (const)
  bool atom_is_current(const z3::expr& e) const
  {
    assert(e.is_const());
    return literal_index.find(e.id()) != literal_index.end();
  }
  // returns true if e is a literal in current. else false
  bool literal_is_current(const z3::expr& e) const
  {
    if (e.is_not())
      return literal_index.find(e.arg(0).id()) != literal_index.end();
    return atom_is_current(e);
  }

  //  not used
  bool literal_is_p(const z3::expr& e) const
  {
    if (e.is_not())
      return literal_index_p.find(e.arg(0).id()) != literal_index.end();
    if (e.is_const())
      return literal_index_p.find(e.id()) != literal_index.end();
    return false;
  }

  // converts a literal in the next state to a literal in the current state
  z3::expr operator()(const z3::expr& e) const
  {
    int index;
    if (e.is_not())
    {
      index = literal_index_p.at(e.arg(0).id());
      return !current[index];
    }
    assert(e.is_const());
    index = literal_index_p.at(e.id());
    return current[index];
  }
  z3::expr operator()(int index) const { return current[index]; }
  z3::expr operator()(const std::string& s) const
  {
    z3::expr e = ctx.bool_const(s.c_str());
    return current[indexof(e)];
  }

  // not used
  z3::expr_vector operator()(const z3::expr_vector& vec) const
  {
    z3::expr_vector vec_now(ctx);
    for (const z3::expr& e : vec)
      vec_now.push_back(operator()(e));
    return vec_now;
  }

  // used internally
  // next state expressions
  z3::expr p(const z3::expr& e) const
  {
    if (e.is_not())
    {
      int index = literal_index.at(e.arg(0).id());
      return !next[index];
    }
    assert(e.is_const());

    int index = literal_index.at(e.id());
    return next[index];
  }
  // not used
  z3::expr p(int index) const { return next[index]; }
  // not used
  z3::expr p(const std::string& s) const
  {
    z3::expr e = ctx.bool_const(s.c_str());
    return next[indexof(e)];
  }

  // converts a vector of literals into a vector of literals in the next state
  // assumes vec is a vector of consts in current
  //! constructs a new vector
  z3::expr_vector p(const z3::expr_vector& vec) const
  {
    z3::expr_vector vec_next(ctx);
    for (const z3::expr& e : vec)
      vec_next.push_back(p(e));
    return vec_next;
  }
  // not used
  z3::expr_vector p(const std::vector<z3::expr>& vec) const
  {
    z3::expr_vector vec_next(ctx);
    for (const z3::expr& e : vec)
      vec_next.push_back(p(e));
    return vec_next;
  }

  // expose vectors for enumeration
  // const expr_vector& are mutable since it only stores a pointer to a
  // container
  z3::expr_vector currents() const { return z3ext::copy(current); }
  z3::expr_vector nexts() const { return z3ext::copy(next); }
  int size() const { return current.size(); }

  void add_literal(const std::string& name)
  {
    assert(!finished);
    if (encodes != Encoding::EXPRESSIONS)
      encodes = Encoding::LITERALS;

    z3::expr lit   = ctx.bool_const(name.c_str());
    z3::expr lit_p = ctx.bool_const((name + ".p").c_str());

    current.push_back(std::move(lit));
    next.push_back(std::move(lit_p));

    literal_index.insert(std::make_pair(lit.id(), current.size() - 1));
    literal_index_p.insert(std::make_pair(lit_p.id(), current.size() - 1));
  }

  void add_expression(z3::expr e, const ExpressionCache& cache)
  {
    assert(!finished);
    assert(cache.encodes == Encoding::LITERALS);
    encodes = Encoding::EXPRESSIONS;

    current.push_back(e);
    z3::expr e_next = e.substitute(cache.currents(), cache.nexts());
    next.push_back(e_next);
  }

  void finish() { finished = true; }

  void show(std::ostream& out) const
  {
    out << "Lits:      " << str::extend::join(current) << std::endl;
    out << "Next Lits: " << str::extend::join(next) << std::endl;
  }
};

#endif
