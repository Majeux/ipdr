#ifndef MY_EXPR_H
#define MY_EXPR_H

#include "z3-ext.h"
#include <bitset>
#include <climits>
#include <fmt/color.h>
#include <z3++.h>
namespace mysat::primed
{
  constexpr size_t MAX_BITS = std::numeric_limits<unsigned>::digits;
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
    BitVec(z3::context& c, const std::string& name, size_t Nbits)
        : size(Nbits), ctx(c), current(ctx), next(ctx)
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

    // construct a bitvector capable of holding the number in max_val
    static BitVec holding(
        z3::context& c, const std::string& name, size_t max_val)
    {
      return BitVec(c, name, std::log2(max_val) + 1);
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

    // equality operator to expression conversions
    z3::expr equals(unsigned n) const { return z3::mk_and(uint(n)); }
    z3::expr p_equals(unsigned n) const { return z3::mk_and(uint_p(n)); }

    // all bits equal in current and next
    z3::expr unchanged() const
    {
      z3::expr_vector conj(ctx);
      for (size_t i = 0; i < size; i++)
        conj.push_back(current[i] == next[i]);

      return z3::mk_and(conj);
    }

    //  N-bit less comparison
    //  returns a formula in cnf
    template <unsigned N> z3::expr less(unsigned n) const
    {
      return z3ext::tseytin::to_cnf(rec_less<N, N>(n));
    }

   private:
    size_t size;
    z3::context& ctx;

    z3::expr_vector current;
    z3::expr_vector next;

    z3::expr_vector unint_to_lits(unsigned n, bool primed) const
    {
      z3::expr_vector rv(ctx), reversed(ctx);
      std::bitset<MAX_BITS> bits(n);
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

    // compare bits 4 bits of of "bv" with 4 bits of "n"
    // 'i' is the most significant bit
    z3::expr less_4b(size_t i, unsigned n) const
    {
      assert(i >= 4 && (i % 4 == 0)); // i == 4, 8, 12, 16
      const std::bitset<MAX_BITS> bits(n);
      std::bitset<MAX_BITS> relevant(0);
      for (size_t j = i - 3; j <= i; j++)
        relevant.set(j);

      auto set = [this, &bits](size_t i) { return ctx.bool_val(bits.test(i)); };

      assert(size <= bits.size());
      if (relevant.to_ulong() == 0)
        return ctx.bool_val(false); // unsigned, so impossible

      z3::expr_vector disj(ctx); // TODO automate pattern
      // clang-format off
      disj.push_back(!current[i] && set(i));
      disj.push_back(!(current[i] ^ set(i)) && !current[i-1] && set(i-1));
      disj.push_back(!(current[i] ^ set(i)) && !(current[i-1] ^ set(i-1)) && !current[i-2] && set(i-2));
      disj.push_back(!(current[i] ^ set(i)) && !(current[i-1] ^ set(i-1)) && !(current[i-2] ^ set(i-2)) && !current[i-3] && set(i-3));
      // clang-format on

      return z3::mk_or(disj);
    }

    // compares the 'Nbits' most significant bits, starting from 'N-1' down
    template <unsigned N, unsigned Nbits> z3::expr eq(unsigned n) const
    {
      const std::bitset<MAX_BITS> bits(n);
      const unsigned last = N - 1;

      z3::expr_vector conj(ctx);
      for (unsigned i = 0; i < Nbits; i++)
      {
        assert(i < size);
        if (bits.test(last - i))
          conj.push_back(current[last - i]);
        else
          conj.push_back(!current[last - i]);
      }

      return z3::mk_and(conj);
    }

    // compares the 'Nbits' most significant bits, starting from 'N-1' down
    template <unsigned N, unsigned Nbits> z3::expr rec_less(unsigned n) const
    {
      if (Nbits == 4)
        return less_4b(N - 1, n);

      assert(Nbits > 4);
      assert((Nbits & (Nbits - 1)) == 0); // Nbits must be a power of 2

      // terminate early
      z3::expr significant_less = rec_less<N, N / 2>(n);
      // compare rest
      z3::expr significant_eq   = eq<N, N / 2>(n);
      z3::expr remainder_less   = rec_less<N / 2, N / 2>(n);

      return significant_less || (significant_eq && remainder_less);
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

#endif // MY_EXPR_H
