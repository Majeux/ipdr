#include "expr.h"
#include "z3-ext.h"
#include <cstdlib>
#include <optional>
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

  bool Lit::extract_value(const z3::expr_vector& cube, lit_type t) const
  {
    // matches name[i], extracts name and i
    std::regex is_value;
    std::smatch match;
    std::string match_name;
    if (t == primed)
    {
      // is_value = (R"(([[:alnum:]_]+).p)"); // primed variable
      is_value = next_name;
      match_name = next_name;
    }
    else
    {
      // is_value = (R"(([[:alnum:]_]+))");
      is_value = name;
      match_name = name;
    }

    std::optional<bool> v;

    for (const expr& l : cube)
    {
      assert(z3ext::is_lit(l));
      std::string l_str = l.to_string();
      // if (std::regex_search(l_str, match, is_value))
      if (l_str.find(match_name) != std::string::npos)
      {
        // assert(match.size() == 2);
        // if (match[1] == match_name)
        // {
          // no contradicting literals
          assert(not v.has_value() || (v == not l.is_not()));
          v = not l.is_not();
        // }
      }
    }

    return v.value();
  }

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

  BitVec::numrep_t BitVec::extract_value(
      const expr_vector& cube, lit_type t) const
  {
    std::bitset<MAX_BITS> n;
    // matches name[i], extracts name and i
    std::regex is_value;
    std::smatch match;
    std::string match_name;
    if (t == primed)
    {
      // is_value = R"(([[:alnum:]_]+).p\[([[:digit:]]+)\])"; // primed variable
      is_value = fmt::format("{}\\[([[:digit:]]+)\\]", next_name);
      match_name = next_name;
    }
    else
    {
      // is_value = R"(([[:alnum:]_]+)\[([[:digit:]]+)\])";
      is_value = fmt::format("{}\\[([[:digit:]]+)\\]", name);
      match_name = name;
    }

    for (const expr& l : cube)
    {
      assert(z3ext::is_lit(l));
      std::string l_str = l.to_string();
      if (std::regex_search(l_str, match, is_value))
      {
        assert(match.size() == 2);
        // if (match[1] != match_name)
        //   continue;

        numrep_t i = std::stoul(match[1]);

        n.set(i, !l.is_not());
      }
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

  expr BitVec::less_4b(size_t msb, numrep_t n) const
  {
    assert(msb < INT_MAX);
    const std::bitset<MAX_BITS> bits(n);
    assert(size <= bits.size());

    std::bitset<MAX_BITS> relevant(0);
    for (size_t i = msb - 3; i <= msb; i++)
      relevant.set(i, bits.test(i));

    if (relevant.to_ulong() == 0)
      return ctx.bool_val(false); // unsigned, so impossible

    auto set = [this, &bits](size_t i) { return ctx.bool_val(bits.test(i)); };

    expr_vector disj(ctx);
    for (size_t i = 0; i <= msb; i++)
    {
      expr e = ctx.bool_val(true);
      for (int j = i; j >= 0; j--)
      {
        // handle if BitVec does not store msb+1 bits
        expr c_bit = j < (int)size ? current[j] : ctx.bool_val(false);
        if (j == 0)
          e = !c_bit && set(j) && e;
        else
          e = !(c_bit ^ set(j)) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  expr BitVec::less_4b(size_t msb, const BitVec& cube) const
  {
    assert(size <= cube.size);

    // disj.push_back(!current[msb] && cube(msb));
    // disj.push_back(!(current[msb] ^ cube(msb)) && !current[msb-1] &&
    // cube(msb-1)); disj.push_back(!(current[msb] ^ cube(msb)) &&
    // !(current[msb-1] ^ cube(msb-1)) && !current[msb-2] && cube(msb-2));
    // disj.push_back(!(current[msb] ^ cube(msb)) && !(current[msb-1] ^
    // cube(msb-1)) && !(current[msb-2] ^ cube(msb-2)) && !current[msb-3] &&
    // cube(msb-3));
    expr_vector disj(ctx);
    for (size_t i = 0; i <= msb; i++)
    {
      expr e = ctx.bool_val(true);
      for (int j = i; j >= 0; j--)
      {
        // handle if BitVec does not store i+1 bits
        expr c_bit = j < (int)size ? current[j] : ctx.bool_val(false);
        if (j == 0)
          e = !c_bit && cube(j) && e;
        else
          e = !(c_bit ^ cube(j)) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  // z3::expr BitVec::rec_less(numrep_t n, size_t msb, size_t nbits) const
  // {
  //   if (nbits == 4)
  //     return less_4b(msb - 1, n);

  //   assert(nbits > 4);
  //   assert((nbits & (nbits - 1)) == 0); // Nbits must be a power of 2

  //   // terminate early
  //   z3::expr significant_less = rec_less(n, msb, msb / 2);
  //   // compare rest
  //   z3::expr significant_eq   = eq(n, msb, msb / 2);
  //   z3::expr remainder_less   = rec_less(n, msb / 2, msb / 2);

  //   return significant_less || (significant_eq && remainder_less);
  // }

  // z3::expr BitVec::rec_less(const BitVec& n, size_t msb, size_t nbits) const
  // {
  //   if (nbits == 4)
  //     return less_4b(msb - 1, n);

  //   assert(nbits > 4);
  //   assert((nbits & (nbits - 1)) == 0); // Nbits must be a power of 2

  //   // terminate early
  //   z3::expr significant_less = rec_less(n, msb, msb / 2);
  //   // compare rest
  //   z3::expr significant_eq   = eq(n, msb, msb / 2);
  //   z3::expr remainder_less   = rec_less(n, msb / 2, msb / 2);

  //   return significant_less || (significant_eq && remainder_less);
  // }

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

  z3::expr BitVec::eq(const BitVec& n, size_t msb, size_t nbits) const
  {
    z3::expr_vector conj(ctx);
    for (size_t i = 0; i < nbits; i++)
    {
      assert(i < size && i < n.size);
      conj.push_back(!(n(msb - i) ^ current[msb - i])); // n[i] <=> current[i]
    }

    return z3::mk_and(conj);
  }

  // Array
  // public
  Array::Array(z3::context& c, const std::string& n, size_t s, size_t bits)
      : size{ s }, n_vals{}, index(c, n + "_i", bits), value(c, n + "_v", bits),
        index_solver(c)
  {
  }

  expr Array::store(size_t i, BitVec::numrep_t v) const
  {
    return z3::implies(mk_and(index.uint(i)), mk_and(value.uint(v)));
  }

  expr Array::store_p(size_t i, BitVec::numrep_t v) const
  {
    return z3::implies(mk_and(index.uint_p(i)), mk_and(value.uint_p(v)));
  }

  // to be appended to an array-statement: a series of store() expressions
  expr Array::contains(size_t i, BitVec::numrep_t v) const
  {
    return mk_and(index.uint(i)) && mk_and(value.uint(v));
  }

  expr Array::contains_p(size_t i, BitVec::numrep_t v) const
  {
    return mk_and(index.uint_p(i)) && mk_and(value.uint_p(v));
  }

  expr Array::cube_idx_store(const expr_vector x, BitVec::numrep_t v) const
  {
    expr_vector conj(x.ctx());

    for (size_t i = 0; i < x.size(); i++)
      conj.push_back(!(x[i] ^ index(i))); // x[i] <=> index[i]
    for (const expr l : value.uint(v))
      conj.push_back(l);

    return mk_and(conj);
  }

  expr Array::cube_idx_store_p(const expr_vector x, BitVec::numrep_t v) const
  {
    expr_vector conj(x.ctx());

    for (size_t i = 0; i < x.size(); i++)
      conj.push_back(!(x[i] ^ index(i))); // x[i] <=> index[i]
    for (const expr l : value.uint(v))
      conj.push_back(l);
    // TODO link to value
    return mk_and(conj);
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
