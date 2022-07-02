#ifndef MY_EXPR_H
#define MY_EXPR_H

#include "z3-ext.h"
#include <bitset>
#include <climits>
#include <fmt/color.h>
#include <z3++.h>
namespace mysat::primed
{

  class IStays
  {
   public:
    virtual z3::expr unchanged() const = 0;
  };

  template <typename Tcontainer> class IPrimed
  {
   public:
    const std::string name;
    const std::string next_name;

    IPrimed(z3::context& c, const std::string& n)
        : name(n), next_name(n + ".p"), ctx(c), current(ctx), next(ctx)
    {
    }

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
    using numrep_t = unsigned;
    static constexpr size_t MAX_BITS = std::numeric_limits<numrep_t>::digits;

    // name = base name for the vector, each bit is "name[0], name[1], ..."
    // max = the maximum (unsigned) integer value the vector should describe
    BitVec(z3::context& c, const std::string& n, size_t Nbits);

    // construct a bitvector capable of holding the number in max_val
    static BitVec holding(z3::context& c, const std::string& n, numrep_t max_val);

    // return all literals comprising the vector
    operator const z3::expr_vector&() const override;
    const z3::expr_vector& operator()() const override;
    const z3::expr_vector& p() const override;

    // all bits equal in current and next
    z3::expr unchanged() const override;

    // access individual literals
    z3::expr operator()(size_t i) const;
    z3::expr p(size_t i) const;

    // uint to bitvector representation
    z3::expr_vector uint(numrep_t n) const;
    z3::expr_vector uint_p(numrep_t n) const;

    // cube to uint conversion
    numrep_t extract_value(const z3::expr_vector& cube) const;

    // equality operator to expression conversions
    z3::expr equals(numrep_t n) const;
    z3::expr p_equals(numrep_t n) const;

    //  N-bit less comparison
    //  returns a formula in cnf
    z3::expr less(numrep_t n) const;

   private:
    size_t size;

    std::string index_str(std::string_view n, size_t i) const;
    z3::expr_vector unint_to_lits(numrep_t n, bool primed) const;

    // compare bits 4 bits of of "bv" with 4 bits of "n"
    // 'i' is the most significant bit
    z3::expr less_4b(size_t i, numrep_t n) const;

    // compares the 'nbits' most significant bits, starting from 'msb' down
    z3::expr eq(numrep_t n, size_t msb, size_t nbits) const;
    // compares the 'Nbits' most significant bits, starting from 'msb' down
    z3::expr rec_less(numrep_t x, size_t msb, size_t nbits) const;
  };

  class Array
  {
   public:
    Array(z3::context& c, const std::string& name, size_t s, size_t bits);

    const size_t size;
    const BitVec::numrep_t n_vals;

    // A{i] <- v
    z3::expr store(unsigned i, unsigned v) const;
    z3::expr store_p(unsigned i, unsigned v) const;
    // A{i] == v
    z3::expr contains(unsigned i, unsigned v) const;
    z3::expr contains_p(unsigned i, unsigned v) const;

    // return a cube representing the value of A[i], expressed in variables of
    // "value"
    z3::expr_vector get_value(const z3::expr& A, size_t i);
    BitVec::numrep_t get_value_uint(const z3::expr& A, size_t i);

   private:

    BitVec index;
    BitVec value;

    z3::solver index_solver;
  }; // class Array
} // namespace mysat::primed

#endif // MY_EXPR_H
