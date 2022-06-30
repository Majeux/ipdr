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

  class IStays
  {
   public:
    virtual z3::expr unchanged() const = 0;
  };

  template <typename Tcontainer> class IPrimed
  {
   public:
    IPrimed(z3::context& c) : ctx(c), current(ctx), next(ctx) {}

    virtual operator const Tcontainer&() const   = 0;
    virtual const Tcontainer& operator()() const = 0;
    virtual const Tcontainer& p() const          = 0;

   protected:
    z3::context& ctx;
    Tcontainer current;
    Tcontainer next;
  }; // class IPrimed

  class Lit final : public IPrimed<z3::expr>, public IStays
  {
   public:
    Lit(z3::context& c, const std::string& name);

    operator const z3::expr&() const override;
    const z3::expr& operator()() const override;
    const z3::expr& p() const override;
    z3::expr unchanged() const override;

  }; // class Lit

  class BitVec final : public IPrimed<z3::expr_vector>, public IStays
  {
   public:
    // name = base name for the vector, each bit is "name[0], name[1], ..."
    // max = the maximum (unsigned) integer value the vector should describe
    BitVec(z3::context& c, const std::string& name, size_t Nbits);

    // construct a bitvector capable of holding the number in max_val
    static BitVec holding(
        z3::context& c, const std::string& name, size_t max_val);

    // return all literals comprising the vector
    operator const z3::expr_vector&() const override;
    const z3::expr_vector& operator()() const override;
    const z3::expr_vector& p() const override;

    // all bits equal in current and next
    z3::expr unchanged() const override;

    // access individual literals
    z3::expr operator()(size_t i) const;
    z3::expr p(size_t i) const;

    // bitvector to unsigned integer conversions
    z3::expr_vector uint(unsigned n) const;
    z3::expr_vector uint_p(unsigned n) const;

    // equality operator to expression conversions
    z3::expr equals(unsigned n) const;
    z3::expr p_equals(unsigned n) const;

    //  N-bit less comparison
    //  returns a formula in cnf
    template <unsigned N> z3::expr less(unsigned n) const;

   private:
    size_t size;

    z3::expr_vector unint_to_lits(unsigned n, bool primed) const;

    // compare bits 4 bits of of "bv" with 4 bits of "n"
    // 'i' is the most significant bit
    z3::expr less_4b(size_t i, unsigned n) const;

    // compares the 'Nbits' most significant bits, starting from 'N-1' down
    template <unsigned N, unsigned Nbits> z3::expr eq(unsigned n) const;
    // compares the 'Nbits' most significant bits, starting from 'N-1' down
    template <unsigned N, unsigned Nbits> z3::expr rec_less(unsigned n) const;
  };

  class Array
  {
   public:
    Array(z3::context& c, const std::string& name, unsigned s, unsigned bits);

    unsigned size() const;

    z3::expr store(unsigned i, unsigned v) const;
    z3::expr store_p(unsigned i, unsigned v) const;
    z3::expr contains(unsigned i, unsigned v) const;

    // return a cube representing the value of A[i], expressed in variables of
    // "value"
    z3::expr_vector get_value(const z3::expr& A, unsigned i);

   private:
    unsigned _size;

    BitVec index;
    BitVec value;

    z3::solver index_solver;
  }; // class Array
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

#include "expr.hpp"

#endif // MY_EXPR_H
