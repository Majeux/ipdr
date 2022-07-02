#include "expr.h"
#include "z3-ext.h"
#include <cstdlib>
#include <regex>

namespace mysat::primed
{
  using z3::expr;
  using z3::expr_vector;
  using z3::mk_and;

  // Lit
  //
  Lit::Lit(z3::context& c, const std::string& n) : IPrimed<expr>(c, n)
  {
    current = ctx.bool_const(name.c_str());
    next    = ctx.bool_const(next_name.c_str());
  }

  Lit::operator const expr&() const { return current; }
  const expr& Lit::operator()() const { return current; }
  const expr& Lit::p() const { return next; }

  expr Lit::unchanged() const { return current == next; }

  // BitVec
  // public
  BitVec::BitVec(z3::context& c, const std::string& n, size_t Nbits)
      : IPrimed<expr_vector>(c, n), size(Nbits)
  {
    for (size_t i = 0; i < size; i++)
    {
      current.push_back(ctx.bool_const(index_str(name, i).c_str()));
      next.push_back(ctx.bool_const(index_str(next_name, i).c_str()));
    }
  }

  BitVec BitVec::holding(z3::context& c, const std::string& n, numrep_t max_val)
  {
    return BitVec(c, n, std::log2(max_val) + 1);
  }

  std::string BitVec::index_str(std::string_view n, size_t i) const
  {
    return fmt::format("{}[{}]", n, i);
  }

  BitVec::operator const expr_vector&() const { return current; }
  const expr_vector& BitVec::operator()() const { return current; }
  const expr_vector& BitVec::p() const { return next; }

  expr BitVec::operator()(size_t i) const { return current[i]; }
  expr BitVec::p(size_t i) const { return next[i]; }

  expr_vector BitVec::uint(numrep_t n) const { return unint_to_lits(n, false); }
  expr_vector BitVec::uint_p(numrep_t n) const
  {
    return unint_to_lits(n, true);
  }

  BitVec::numrep_t BitVec::extract_value(const expr_vector& cube) const
  {
    std::bitset<MAX_BITS> n;
    // matches name[i], extracts name and i
    std::regex is_value(R"(([[:alnum:]_]+)\[([[:digit:]]+)\])");
    std::smatch match;

    for (const expr l : cube)
    {
      assert(z3ext::is_lit(l));
      {
        std::string l_str = l.to_string();
        assert(std::regex_search(l_str, match, is_value));
        assert(match.size() == 3);
      }

      if (match[1] != name)
        continue;

      numrep_t i = std::stoul(match[2]);

      n.set(i, !l.is_not());
    }

    return n.to_ulong();
  }

  expr BitVec::equals(numrep_t n) const { return mk_and(uint(n)); }
  expr BitVec::p_equals(numrep_t n) const { return mk_and(uint_p(n)); }

  expr BitVec::unchanged() const
  {
    expr_vector conj(ctx);
    for (size_t i = 0; i < size; i++)
      conj.push_back(current[i] == next[i]);

    return mk_and(conj);
  }

  // private
  expr_vector BitVec::unint_to_lits(numrep_t n, bool primed) const
  {
    expr_vector rv(ctx), reversed(ctx);
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

  expr BitVec::less_4b(size_t i, numrep_t n) const
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

    expr_vector disj(ctx); // TODO automate pattern
    // clang-format off
      disj.push_back(!current[i] && set(i));
      disj.push_back(!(current[i] ^ set(i)) && !current[i-1] && set(i-1));
      disj.push_back(!(current[i] ^ set(i)) && !(current[i-1] ^ set(i-1)) && !current[i-2] && set(i-2));
      disj.push_back(!(current[i] ^ set(i)) && !(current[i-1] ^ set(i-1)) && !(current[i-2] ^ set(i-2)) && !current[i-3] && set(i-3));
    // clang-format on

    return z3::mk_or(disj);
  }

  z3::expr BitVec::less(numrep_t x) const
  {
    return rec_less(x, size - 1, size - 1);
  }

  z3::expr BitVec::rec_less(numrep_t n, size_t msb, size_t nbits) const
  {
    if (nbits == 4)
      return less_4b(msb - 1, n);

    assert(nbits > 4);
    assert((nbits & (nbits - 1)) == 0); // Nbits must be a power of 2

    // terminate early
    z3::expr significant_less = rec_less(n, msb, msb / 2);
    // compare rest
    z3::expr significant_eq   = eq(n, msb, msb / 2);
    z3::expr remainder_less   = rec_less(n, msb / 2, msb / 2);

    return significant_less || (significant_eq && remainder_less);
  }

  z3::expr BitVec::eq(numrep_t n, size_t msb, size_t nbits) const
  {
    const std::bitset<MAX_BITS> bits(n);

    z3::expr_vector conj(ctx);
    for (size_t i = 0; i < nbits; i++)
    {
      assert(i < size);
      if (bits.test(msb - i))
        conj.push_back(current[msb - i]);
      else
        conj.push_back(!current[msb - i]);
    }

    return z3::mk_and(conj);
  }
  // Array
  // public
  Array::Array(z3::context& c, const std::string& n, size_t s, size_t bits)
      : size{s}, n_vals{}, index(c, n + "_i", bits), value(c, n + "_v", bits),
        index_solver(c)
  {
  }


  expr Array::store(unsigned i, unsigned v) const
  {
    return z3::implies(mk_and(index.uint(i)), mk_and(value.uint(v)));
  }

  expr Array::store_p(unsigned i, unsigned v) const
  {
    return z3::implies(mk_and(index.uint_p(i)), mk_and(value.uint_p(v)));
  }

  // to be appended to an array-statement: a series of store() expressions
  expr Array::contains(unsigned i, unsigned v) const
  {
    return mk_and(index.uint(i)) && mk_and(value.uint(v));
  }

  expr Array::contains_p(unsigned i, unsigned v) const
  {
    return mk_and(index.uint_p(i)) && mk_and(value.uint_p(v));
  }

  expr_vector Array::get_value(const expr& A, size_t i)
  {
    index_solver.reset();
    index_solver.add(A);
    z3::check_result r = index_solver.check(index.uint(i));

    assert(r == z3::check_result::sat);
    expr_vector witness = z3ext::solver::get_witness(index_solver);
    z3ext::sort_cube(witness);

    return witness;
  }

  BitVec::numrep_t Array::get_value_uint(const expr& A, size_t i)
  {
    return value.extract_value(get_value(A, i));
  }
} // namespace mysat::primed
