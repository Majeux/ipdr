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

  // this class can return an expression that ensures its value does not change
  // in the next state
  class IStays
  {
   public:
    virtual z3::expr unchanged() const = 0;
  };

  // this class can return the names of its variables as strings
  class INamed
  {
   public:
    virtual std::vector<std::string> names() const   = 0;
    virtual std::vector<std::string> names_p() const = 0;
  };

  template <typename Tcontainer> class IPrimed : public INamed
  {
   public:
    const std::string name;
    const std::string next_name;

    IPrimed(z3::context& c, const std::string& n)
        : name(n), next_name(prime(n)), ctx(c), current(ctx), next(ctx)
    {
    }

    z3::context& get_ctx() const { return ctx; }

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
    std::vector<std::string> names() const override;
    std::vector<std::string> names_p() const override;

    bool extract_value(const z3::expr_vector& cube, lit_type t = base) const;

  }; // class Lit

  // a of vector propositional variables
  class VarVec final : public IPrimed<z3::expr_vector>
  {
   public:
    VarVec(z3::context& c, const std::vector<std::string>& varnames);

    // add literals with given names and automatically generate next-state vars
    void add(const std::vector<std::string>& varnames);
    // add literals with given names and automatically generate next-state vars
    void add(const std::vector<std::string>& currnames,
        const std::vector<std::string>& nextnames);

    operator const z3::expr_vector&() const override;
    const z3::expr_vector& operator()() const override;
    const z3::expr_vector& p() const override;
    std::vector<std::string> names() const override;
    std::vector<std::string> names_p() const override;

    z3::expr operator()(size_t i) const;
    z3::expr p(size_t i) const;
    z3::expr operator()(const z3::expr& e) const;
    z3::expr p(const z3::expr& e) const;
    // convert expressions to and form current and next
    z3::expr_vector operator()(const z3::expr_vector& ev) const;
    z3::expr_vector p(const z3::expr_vector& ev) const;

    bool lit_is_current(const z3::expr& e) const;
    bool lit_is_p(const z3::expr& e) const;

   private:
    // std::unordered_map<z3::expr, z3::expr, z3ext::expr_hash> to_current;
    // std::unordered_map<z3::expr, z3::expr, z3ext::expr_hash> to_next;
    // maps id() of a variable to its ndex in its current/next vector
    std::unordered_map<unsigned, size_t> to_current;
    std::unordered_map<unsigned, size_t> to_next;

  }; // class VarVec

  // a vector of boolean expressionos
  // initialized with a vector of vars that describe the expressions
  // expressions are added after construction
  class ExpVec final : public IPrimed<z3::expr_vector>
  {
   public:
    ExpVec(z3::context& c, const VarVec& v);

    operator const z3::expr_vector&() const override;
    const z3::expr_vector& operator()() const override;
    const z3::expr_vector& p() const override;
    std::vector<std::string> names() const override;
    std::vector<std::string> names_p() const override;

    // add and automatically generate next state
    ExpVec& add(z3::expr e);
    // add current and next state explicitly
    ExpVec& add(const z3::expr& curr, const z3::expr& next);
    // ensure that no further expressions can be added
    void finish();

   private:
    const VarVec& vars;
    bool finished{ false };

  }; // class ExpVec

  class BitVec final : public IPrimed<z3::expr_vector>, public IStays
  {
   public:
    using numrep_t                   = unsigned;
    static constexpr size_t MAX_BITS = std::numeric_limits<numrep_t>::digits;

    const size_t size;

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
    std::vector<std::string> names() const override;
    std::vector<std::string> names_p() const override;

    // all bits equal in current and next
    z3::expr unchanged() const override;

    // access individual literals
    z3::expr operator()(size_t i) const;
    z3::expr p(size_t i) const;

    // uint to bitvector representation
    // * CNF
    z3::expr_vector uint(numrep_t n) const;
    z3::expr_vector uint_p(numrep_t n) const;
    z3::expr_vector uint_both(numrep_t n) const;

    // extract uint representation from relevant literals in a cube
    numrep_t extract_value(
        const z3::expr_vector& cube, const lit_type t = base) const;

    // equality operator to expression conversions
    // * CNF
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

} // namespace mysat::primed

#endif // MY_EXPR_H
