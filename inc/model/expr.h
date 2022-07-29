#ifndef MY_EXPR_H
#define MY_EXPR_H

#include "z3-ext.h"
#include <bitset>
#include <climits>
#include <fmt/color.h>
#include <z3++.h>
namespace mysat::primed
{

  enum lit_type
  {
    base,
    primed
  };

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
        : name(n), next_name(prime(n)), ctx(c), current(ctx), next(ctx)
    {
    }

    virtual operator const Tcontainer&() const   = 0;
    virtual const Tcontainer& operator()() const = 0;
    virtual const Tcontainer& p() const          = 0;

   protected:
    z3::context& ctx;
    Tcontainer current;
    Tcontainer next;

    std::string prime(std::string_view s) const
    {
      assert(s.size() > 0);
      assert(s.size() < 2 || s.substr(s.size() - 2, 2) != ".p");
      return std::string(s) + ".p";
    }
  }; // class IPrimed

  class Lit final : public IPrimed<z3::expr>, public IStays
  {
   public:
    Lit(z3::context& c, const std::string& name);

    operator const z3::expr&() const override;
    const z3::expr& operator()() const override;
    const z3::expr& p() const override;
    z3::expr unchanged() const override;

    bool extract_value(const z3::expr_vector& cube, lit_type t = base) const;

  }; // class Lit

  class LitVec final : public IPrimed<z3::expr_vector>
  {
   public:
    LitVec(z3::context& c, const std::vector<std::string> names);

    operator const z3::expr_vector&() const override;
    const z3::expr_vector& operator()() const override;
    const z3::expr_vector& p() const override;

    z3::expr operator()(const z3::expr& e) const;
    z3::expr p(const z3::expr& e) const;

    bool lit_is_current(const z3::expr& e) const;
    bool lit_is_p(const z3::expr& e) const;

   private:
    std::unordered_map<z3::expr, z3::expr, z3ext::expr_hash> to_current;
    std::unordered_map<z3::expr, z3::expr, z3ext::expr_hash> to_next;

  }; // class LitVec

  class BitVec final : public IPrimed<z3::expr_vector>, public IStays
  {
   public:
    using numrep_t                   = unsigned;
    static constexpr size_t MAX_BITS = std::numeric_limits<numrep_t>::digits;

    // name = base name for the vector, each bit is "name[0], name[1], ..."
    // max = the maximum (unsigned) integer value the vector should describe
    BitVec(z3::context& c, const std::string& n, size_t Nbits);

    // construct a bitvector capable of holding the number in max_val
    static BitVec holding(
        z3::context& c, const std::string& n, numrep_t max_val);

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
    numrep_t extract_value(
        const z3::expr_vector& cube, lit_type t = base) const;

    // equality operator to expression conversions
    z3::expr equals(numrep_t n) const;
    z3::expr p_equals(numrep_t n) const;

    //  N-bit less comparison
    //  returns a formula in cnf
    template <typename Tnum> z3::expr less(const Tnum& n) const
    {
      static_assert(std::is_same<Tnum, numrep_t>::value ||
                        std::is_same<Tnum, BitVec>::value,
          "Number must either be respresented by an unsigned or a cube");
      // less_4b assigns default values if size is too smalle
      size_t nbits = size + (4 - size % 4);
      return rec_less(n, nbits - 1, nbits);
    }

   private:
    size_t size;

    std::string index_str(std::string_view n, size_t i) const;
    z3::expr_vector unint_to_lits(numrep_t n, bool primed) const;

    // compare bits 4 bits of of "bv" with 4 bits of "n"
    // 'i' is the most significant bit
    z3::expr less_4b(numrep_t n, size_t msb) const;
    z3::expr less_4b(const BitVec& cube, size_t msb) const;

    // compares the 'nbits' most significant bits, starting from 'msb' down
    z3::expr eq(numrep_t n, size_t msb, size_t nbits) const;
    z3::expr eq(const BitVec& n, size_t msb, size_t nbits) const;
    // compares the 'Nbits' most significant bits, starting from 'msb' down
    template <typename Tnum>
    z3::expr rec_less(const Tnum& n, size_t msb, size_t nbits) const
    {
      static_assert(std::is_same<Tnum, numrep_t>::value ||
                        std::is_same<Tnum, BitVec>::value,
          "Number must either be respresented by an unsigned or a cube");

      assert(nbits % 4 == 0);
      if (nbits == 4)
        return less_4b(n, msb);
      else if ((nbits & (nbits - 1)) != 0)          // is not a power of 2
        nbits += std::pow(2, std::log2(nbits) + 1); // next power of 2

      assert(nbits > 4);
      assert((nbits & (nbits - 1)) == 0); // Nbits must be a power of 2
      // msb must be in second half, otherwise select a smaller nbits in call
      assert(msb >= nbits / 2);

      // terminate early
      z3::expr significant_less = rec_less(n, msb, nbits / 2);
      // compare rest
      z3::expr significant_eq   = eq(n, msb, nbits / 2);
      z3::expr remainder_less   = rec_less(n, nbits / 2 - 1, nbits / 2);

      return significant_less || (significant_eq && remainder_less);
    }
    // z3::expr rec_less(const BitVec& x, size_t msb, size_t nbits) const;
  };

  class Array
  {
   public:
    using numrep_t = BitVec::numrep_t;

    const size_t size;
    const BitVec::numrep_t n_vals;

    Array(z3::context& c, const std::string& name, size_t s, size_t bits);

    // A{i] <- v
    z3::expr store(size_t i, BitVec::numrep_t v) const;
    z3::expr store_p(size_t i, BitVec::numrep_t v) const;
    // A{i] == v
    z3::expr contains(size_t i, BitVec::numrep_t v) const;
    z3::expr contains_p(size_t i, BitVec::numrep_t v) const;

    // A[x] <- v, where x is a cube
    z3::expr cube_idx_store(const z3::expr_vector x, BitVec::numrep_t v) const;
    z3::expr cube_idx_store_p(
        const z3::expr_vector x, BitVec::numrep_t v) const;

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
