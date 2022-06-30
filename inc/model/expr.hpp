#include <z3++.h>

// #include "expr.h" // uncomment for coc syntax check

namespace mysat::primed
{
  template <unsigned N> z3::expr BitVec::less(unsigned n) const
  {
    return z3ext::tseytin::to_cnf(rec_less<N, N>(n));
  }

  template <unsigned N, unsigned Nbits> z3::expr BitVec::eq(unsigned n) const
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
  template <unsigned N, unsigned Nbits>
  z3::expr BitVec::rec_less(unsigned n) const
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
} // namespace mysat::primed
