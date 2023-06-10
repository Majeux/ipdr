#include "expr.h"
#include "z3-ext.h"
#include <cassert>
#include <cstdlib>
#include <fmt/core.h>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <z3++.h>

namespace mysat::primed
{
  using fmt::format;
  using std::invalid_argument;
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::mk_and;
  using z3ext::strip_not;

  namespace
  {
    vector<string> extract_names(const expr_vector& v)
    {
      vector<string> rv;
      rv.reserve(v.size());
      std::transform(v.begin(), v.end(), std::back_inserter(rv),
          [](expr l) { return l.to_string(); });
      return rv;
    }
  } // namespace

  // Lit
  //
  Lit::Lit(z3::context& c, const string& n) : IPrimed<expr>(c, n)
  {
    z3ext::constrained_cube::validate_lit_name(n);
    current = ctx.bool_const(name.c_str());
    next    = ctx.bool_const(next_name.c_str());
  }

  Lit::operator const expr&() const { return current; }
  const expr& Lit::operator()() const { return current; }
  const expr& Lit::p() const { return next; }
  const expr& Lit::get(lit_type t) const
  {
    if (t == lit_type::base)
      return current;
    return next;
  }

  expr Lit::unchanged() const { return current == next; }

  vector<string> Lit::names() const { return { current.to_string() }; }
  vector<string> Lit::names_p() const { return { next.to_string() }; }

  bool Lit::extract_value(const z3::expr_vector& cube, lit_type t) const
  {
    // matches name[i], extracts name and i
    std::regex is_value;
    string match_name;
    if (t == lit_type::primed)
    {
      is_value   = next_name;
      match_name = next_name;
    }
    else
    {
      is_value   = fmt::format("{}(?!.p)", name); // name not followed by .p
      match_name = name;
    }

    std::optional<bool> v;

    for (const expr& l : cube)
    {
      assert(z3ext::is_lit(l));
      std::smatch match;
      string l_str = l.to_string();
      if (std::regex_search(l_str, match, is_value))
      {
        assert(match.size() == 1);
        // no contradicting literals
        assert(not v.has_value() || (v == not l.is_not()));
        v = not l.is_not();
        // }
      }
    }

    return v.value_or(false); // no value if no literal was found, assumes false
  }

  // VarVec
  // public
  invalid_argument invalid_lit(expr const& l)
  {
    return invalid_argument(
        format("{} is not a literal in the VarVec", l.to_string()));
  }

  VarVec::VarVec(z3::context& c, const vector<string>& names)
      : IPrimed<expr_vector>(c, "varvec")
  {
    add(names);
  }

  void VarVec::add(const vector<string>& names)
  {
    for (const string& n : names)
    {
      z3ext::constrained_cube::validate_lit_name(n);
      const expr new_curr = ctx.bool_const(n.c_str());
      const expr new_next = ctx.bool_const(prime(n).c_str());

      current.push_back(new_curr);
      next.push_back(new_next);

      to_current.emplace(new_next.id(), current.size() - 1);
      to_next.emplace(new_curr.id(), next.size() - 1);
    }
  }

  void VarVec::add(
      const vector<string>& currnames, const vector<string>& nextnames)
  {
    assert(currnames.size() == nextnames.size());

    for (size_t i{ 0 }; i < currnames.size(); i++)
    {
      z3ext::constrained_cube::validate_lit_name(currnames[i]);
      const string& n_curr = currnames[i];
      const string& n_next = nextnames[i];

      const expr new_curr = ctx.bool_const(n_curr.c_str());
      const expr new_next = ctx.bool_const(n_next.c_str());

      current.push_back(new_curr);
      next.push_back(new_next);

      to_current.emplace(new_next.id(), current.size() - 1);
      to_next.emplace(new_curr.id(), next.size() - 1);
    }
  }

  VarVec::operator const expr_vector&() const { return current; }
  const expr_vector& VarVec::operator()() const { return current; }
  const expr_vector& VarVec::p() const { return next; }
  const expr_vector& VarVec::get(lit_type t) const
  {
    if (t == lit_type::base)
      return current;
    return next;
  }
  vector<string> VarVec::names() const { return extract_names(current); }
  vector<string> VarVec::names_p() const { return extract_names(next); }

  expr VarVec::operator()(size_t i) const
  {
    assert(i < current.size());
    return current[i];
  }
  expr VarVec::p(size_t i) const
  {
    assert(i < next.size());
    return next[i];
  }

  expr VarVec::operator()(const expr& e) const
  {
    if (e.is_not())
    {
      expr var = e.arg(0);
      assert(var.is_const());
      auto index = to_current.find(var.id());
      if (index == to_current.end())
        throw invalid_lit(var);
      return !current[index->second];
    }
    assert(e.is_const());
    auto index = to_current.find(e.id());
    if (index == to_current.end())
      return e;
    else
      return current[index->second];
  }

  expr VarVec::p(const expr& e) const
  {
    if (e.is_not())
    {
      expr var = e.arg(0);
      assert(var.is_const());
      auto index = to_next.find(var.id());
      if (index == to_next.end())
        return e;
      return !next[index->second];
    }
    assert(e.is_const());
    auto index = to_next.find(e.id());
    if (index == to_next.end())
      return e;
    return next[index->second];
  }

  expr_vector VarVec::operator()(const expr_vector& ev) const
  {
    expr_vector rv(ctx);
    for (const expr& e : ev)
      rv.push_back(operator()(e));
    return rv;
  }

  expr_vector VarVec::p(const expr_vector& ev) const
  {
    expr_vector rv(ctx);
    for (const expr& e : ev)
      rv.push_back(p(e));
    return rv;
  }

  expr_vector VarVec::p(vector<expr> const& ev) const
  {
    expr_vector rv(ctx);
    for (const expr& e : ev)
      rv.push_back(p(e));
    return rv;
  }

  bool VarVec::lit_is_current(const z3::expr& e) const
  {
    expr key = strip_not(e);
    return to_next.find(key.id()) != to_next.end();
  }

  bool VarVec::lit_is_p(const z3::expr& e) const
  {
    bool rv{ false };
    try
    {
      expr key = strip_not(e);
      rv       = to_current.find(key.id()) != to_current.end();
    }
    catch (const std::invalid_argument& e)
    {
      return false;
    }
    return rv;
  }

  // ExpVec
  // public

  ExpVec::ExpVec(z3::context& c, const VarVec& v)
      : IPrimed<expr_vector>(c, "expvec"), vars(v)
  {
  }

  ExpVec::operator const expr_vector&() const { return current; }
  const expr_vector& ExpVec::operator()() const { return current; }
  const expr_vector& ExpVec::p() const { return next; }
  const expr_vector& ExpVec::get(lit_type t) const
  {
    if (t == lit_type::base)
      return current;
    return next;
  }
  std::vector<z3::expr> ExpVec::p_vec() const { return z3ext::convert(next); }

  vector<string> ExpVec::names() const { return extract_names(current); }
  vector<string> ExpVec::names_p() const { return extract_names(next); }

  ExpVec& ExpVec::add(z3::expr e)
  {
    assert(!finished);

    current.push_back(e);
    z3::expr e_p = e.substitute(vars(), vars.p());
    next.push_back(e_p);

    return *this;
  }

  ExpVec& ExpVec::add(const z3::expr& e, const z3::expr& e_next)
  {
    assert(!finished);

    current.push_back(e);
    next.push_back(e_next);

    return *this;
  }

  void ExpVec::finish() { finished = true; }

  // BitVec
  // public
  //
  namespace
  {
    string index_str(std::string_view n, size_t i)
    {
      return fmt::format("{}__{}", n, i);
    }

    string carry_str(std::string_view n, size_t i)
    {
      return fmt::format("{}__carry{}", n, i);
    }
  } // namespace

  BitVec::BitVec(z3::context& c)
      : IPrimed<expr_vector>(c, "empty"), size(0), carry_out(c)
  {
  }

  BitVec::BitVec(z3::context& c, const string& n, size_t Nbits)
      : IPrimed<expr_vector>(c, n), size(Nbits), carry_out(c)
  {
    z3ext::constrained_cube::validate_lit_name(n);
    for (size_t i = 0; i < size; i++)
    {
      current.push_back(ctx.bool_const(index_str(name, i).c_str()));
      next.push_back(ctx.bool_const(index_str(next_name, i).c_str()));
    }
  }

  BitVec BitVec::holding(z3::context& c, const string& n, numrep_t max_val)
  {
    size_t bits = std::log2(max_val) + 1; // floored
    assert(bits <= std::numeric_limits<numrep_t>::digits);
    return BitVec(c, n, bits);
  }

  BitVec& BitVec::incrementable()
  {
    carry_out.resize(0);
    for (size_t i = 0; i < size; i++)
    {
      carry_out.push_back(ctx.bool_const(carry_str(name, i).c_str()));
    }
    return *this;
  }

  BitVec::operator const expr_vector&() const { return current; }
  const expr_vector& BitVec::operator()() const { return current; }
  const expr_vector& BitVec::p() const { return next; }
  const expr_vector& BitVec::get(lit_type t) const
  {
    if (t == lit_type::base)
      return current;
    return next;
  }

  expr BitVec::operator()(size_t i) const { return current[i]; }
  expr BitVec::p(size_t i) const { return next[i]; }

  vector<string> BitVec::names() const { return extract_names(current); }
  vector<string> BitVec::names_p() const { return extract_names(next); }

  expr_vector BitVec::uint(numrep_t n) const
  {
    return unint_to_lits(n, lit_type::base);
  }
  expr_vector BitVec::uint_p(numrep_t n) const
  {
    return unint_to_lits(n, lit_type::primed);
  }
  expr_vector BitVec::uint_both(numrep_t n) const
  {
    expr_vector rv = uint(n);
    for (const expr& e : uint_p(n))
      rv.push_back(e);
    return rv;
  }

  BitVec::numrep_t BitVec::extract_value(
      const expr_vector& cube, const lit_type t) const
  {
    std::bitset<MAX_BITS> n;
    const string match_name{ t == lit_type::primed ? next_name : name };
    // matches name__i, extracts i
    const std::regex is_value{ fmt::format("{}__([[:digit:]]+)", match_name) };

    for (const expr& l : cube)
    {
      assert(z3ext::is_lit(l)); // vector represents a cube
      std::smatch match;
      const string l_str = l.to_string();
      if (std::regex_search(l_str, match, is_value))
      {
        assert(match.size() == 2);
        const numrep_t i = std::stoul(match[1]);
        n.set(i, !l.is_not());
      }
    }

    return n.to_ulong();
  }

  namespace
  {
    // equality for literals
    expr eq_cnf(expr const& a, expr const& b)
    {
      if (!a.is_const())
        throw std::invalid_argument("a must be a const.");
      if (!b.is_const())
        throw std::invalid_argument("b must be a const.");
      return (!a || b) && (a || !b);
    }
    expr neq_cnf(expr const& a, expr const& b)
    {
      if (!a.is_const())
        throw std::invalid_argument("a must be a const.");
      if (!b.is_const())
        throw std::invalid_argument("b must be a const.");
      return (!a || !b) && (a || b);
    }
  } // namespace

  expr BitVec::equals(numrep_t n) const { return mk_and(uint(n)); }
  expr BitVec::p_equals(numrep_t n) const { return mk_and(uint_p(n)); }
  expr BitVec::equals(expr_vector const& other) const
  {
    if (size != other.size())
      throw std::invalid_argument("Compared BitVecs must be of equal size");

    expr_vector rv(ctx);
    for (size_t i{ 0 }; i < size; i++)
      rv.push_back(eq_cnf(current[i], other[i]));

    return z3::mk_and(rv).simplify();
  }

  expr BitVec::p_equals(expr_vector const& other) const
  {
    if (size != other.size())
      throw std::invalid_argument("Compared BitVecs must be of equal size");

    expr_vector rv(ctx);
    for (size_t i{ 0 }; i < size; i++)
      rv.push_back(eq_cnf(next[i], other[i]));

    return z3::mk_and(rv).simplify();
  }

  expr BitVec::nequals(expr_vector const& other) const
  {
    if (size != other.size())
      throw std::invalid_argument("Compared BitVecs must be of equal size");

    expr_vector rv(ctx);
    for (size_t i{ 0 }; i < size; i++)
      rv.push_back(neq_cnf(current[i], other[i]));

    return z3::mk_or(rv).simplify();
  }

  expr BitVec::p_nequals(expr_vector const& other) const
  {
    if (size != other.size())
      throw std::invalid_argument("Compared BitVecs must be of equal size");

    expr_vector rv(ctx);
    for (size_t i{ 0 }; i < size; i++)
      rv.push_back(neq_cnf(next[i], other[i]));

    return z3::mk_or(rv).simplify();
  }

  expr BitVec::unchanged() const
  {
    expr_vector conj(ctx);
    for (size_t i = 0; i < size; i++) // forall i: c[i] == n[i]
      conj.push_back(eq_cnf(current[i], next[i]));

    return mk_and(conj);
  }

  z3::expr BitVec::incremented() const
  {
    if (carry_out.empty())
      throw std::runtime_error("This BitVec was not marked as incrementable, "
                               "but has been incremented().");

    // ripple-carry adder with constant operant 0001
    expr_vector out(ctx);

    // half-adder A+1 [carry 0]:
    // next[0] == !current[0], carry_out[0] == current[0]
    out.push_back(neq_cnf(next[0], current[0]));
    out.push_back(eq_cnf(carry_out[0], current[0]));
    // full-adder A+0 [carry]:
    for (size_t i{ 1 }; i < size; i++)
    {
      out.push_back( // n == (a xor c)
          (!current[i] || !carry_out[i - 1] || !next[i]) &&
          (!current[i] || carry_out[i - 1] || next[i]) &&
          (current[i] || !carry_out[i - 1] || next[i]) &&
          (current[i] || carry_out[i - 1] || !next[i]));
      out.push_back( // carry_out' == (current and carry_out)
          (!current[i] || !carry_out[i - 1] || carry_out[i]) &&
          (current[i] || !carry_out[i]) && (carry_out[i - 1] || !carry_out[i]));
    }

    return z3::mk_and(out);
  }

  // private
  expr_vector BitVec::unint_to_lits(numrep_t n, lit_type t) const
  {
    expr_vector rv(ctx), reversed(ctx);
    std::bitset<MAX_BITS> bits(n);
    assert(size <= bits.size());

    for (size_t i = 0; i < current.size(); i++)
    {
      if (n & 1) // first bit is 1
        reversed.push_back(get(t)[i]);
      else
        reversed.push_back(!get(t)[i]);

      n >>= 1;
    }

    for (int i = reversed.size() - 1; i >= 0; i--)
      rv.push_back(reversed[i]);

    return rv;
  }

  expr BitVec::less_4b(numrep_t n, size_t msb, lit_type t) const
  {
    assert(msb < INT_MAX);
    assert((msb + 1) % 4 == 0);
    size_t lsb = msb - 3;

    const std::bitset<MAX_BITS> bits(n);
    assert(size <= bits.size());

    std::bitset<MAX_BITS> relevant(0);
    for (size_t i = lsb; i <= msb; i++)
      relevant.set(i, bits.test(i));

    if (relevant.to_ulong() == 0)
      return ctx.bool_val(false); // unsigned, so impossible

    auto set = [this, &bits](size_t i) { return ctx.bool_val(bits.test(i)); };

    expr_vector disj(ctx);
    for (int i = msb; i >= (int)lsb; i--)
    {
      expr bit = i < (int)size ? get(t)[i] : ctx.bool_val(false);
      expr e   = !bit && set(i);
      for (size_t j = i + 1; j <= msb; j++)
      {
        // handle if BitVec does not store i+1 bits
        expr bit = j < size ? get(t)[j] : ctx.bool_val(false);
        e        = eq_cnf(bit, set(j)) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  expr BitVec::less_4b(const BitVec& cube, size_t msb, lit_type t) const
  {
    assert(size == cube.size);
    assert(msb < INT_MAX);
    assert((msb + 1) % 4 == 0);
    size_t lsb = msb - 3;

    expr_vector disj(ctx);
    for (int i = msb; i >= (int)lsb; i--)
    {
      expr bit      = i < (int)size ? get(t)[i] : ctx.bool_val(false);
      expr cube_bit = i < (int)size ? cube(i) : ctx.bool_val(false);
      expr e        = !bit && cube_bit;
      for (size_t j = i + 1; j <= msb; j++)
      {
        // handle if BitVec does not store i+1 bits
        expr bit      = j < size ? get(t)[j] : ctx.bool_val(false);
        expr cube_bit = j < size ? cube(j) : ctx.bool_val(false);
        e             = eq_cnf(bit, cube_bit) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  z3::expr BitVec::eq(numrep_t n, size_t msb, size_t nbits, lit_type t) const
  {
    assert((msb + 1) % 4 == 0);
    assert((msb + 1) >= nbits);
    size_t lsb = msb - (nbits - 1);

    const std::bitset<MAX_BITS> bits(n);

    z3::expr_vector conj(ctx);
    for (size_t i = lsb; i <= msb; i++)
    {
      expr bit = i < size ? get(t)[i] : ctx.bool_val(false);
      if (bits.test(i))
        conj.push_back(bit);
      else
        conj.push_back(!bit);
    }

    return z3::mk_and(conj);
  }

  z3::expr BitVec::eq(
      const BitVec& cube, size_t msb, size_t nbits, lit_type t) const
  {
    assert(size == cube.size);
    assert((msb + 1) % 4 == 0);
    assert((msb + 1) >= nbits);
    size_t lsb = msb - (nbits - 1);

    z3::expr_vector conj(ctx);
    for (size_t i = lsb; i <= msb; i++)
    {
      expr bit      = i < size ? get(t)[i] : ctx.bool_val(false);
      expr cube_bit = i < size ? cube(i) : ctx.bool_val(false);
      conj.push_back(eq_cnf(cube_bit, bit)); // n[i] == current[i]
    }

    return z3::mk_and(conj);
  }

  // TESTS
  //
  void bv_comp_test(size_t max_value)
  {
    std::cout << "bitvector comparison test" << std::endl;
    z3::context ctx;
    auto bv1 = BitVec::holding(ctx, "b1", max_value);
    auto bv2 = BitVec::holding(ctx, "b2", max_value);
    unsigned wrong{ 0 };

    z3::solver s(ctx);
    s.add(bv1.less(bv2).simplify());

    std::cout << s.assertions() << std::endl;
    std::cout << "begin" << std::endl;
    for (unsigned i = 0; i <= max_value; i++)
    {
      for (unsigned j = 0; j <= max_value; j++)
      {
        z3::expr_vector ass(ctx);
        ass.push_back(bv1.equals(i));
        ass.push_back(bv2.equals(j));
        z3::check_result r = s.check(ass);
        if (i >= j && r == z3::check_result::sat)
        {
          std::cout << fmt::format("{} < {}", i, j) << std::endl
                    << "\tfalse sat" << std::endl
                    << s << std::endl
                    << "---" << std::endl;
          wrong++;
        }
        if (i < j && r == z3::check_result::unsat)
        {
          std::cout << fmt::format("{} < {}", i, j) << std::endl
                    << "\tfalse unsat" << std::endl
                    << s << std::endl
                    << "---" << std::endl;
          wrong++;
        }
      }
    }
    std::cout << fmt::format("{} wrong bv comparisons", wrong) << std::endl;
    return;
  }

  void bv_val_test(size_t max_value)
  {
    z3::context ctx;
    auto bv = BitVec::holding(ctx, "b", max_value);
    unsigned wrong{ 0 };

    for (unsigned i = 0; i <= max_value; i++)
    {
      for (unsigned j = 0; j <= max_value; j++)
      {
        z3::solver s(ctx);

        s.add(bv.equals(i));
        s.add(bv.less(j));
        z3::check_result r = s.check();
        if (i >= j && r == z3::check_result::sat)
        {
          std::cout << fmt::format("{} < {}", i, j) << std::endl
                    << "\tfalse sat" << std::endl
                    << s << std::endl
                    << "---" << std::endl;
          wrong++;
        }
        if (i < j && r == z3::check_result::unsat)
        {
          std::cout << fmt::format("{} < {}", i, j) << std::endl
                    << "\tfalse unsat" << std::endl
                    << s << std::endl
                    << "---" << std::endl;
          wrong++;
        }
      }
    }
    std::cout << fmt::format("{} wrong bv - uint comparisons", wrong)
              << std::endl;
    return;
  }

  void bv_inc_test(size_t max_value)
  {
    z3::context ctx;
    auto bv = BitVec::holding(ctx, "b", max_value);
    unsigned wrong{ 0 };

    std::cout << "Adder" << std::endl;
    for (unsigned i = 0; i <= max_value; i++)
    {
      z3::solver s(ctx);

      s.add(bv.equals(i));
      s.add(bv.incremented());

      std::optional<z3::expr_vector> result = z3ext::solver::check_witness(s);
      if (result)
      {
        unsigned sum = bv.extract_value(result.value(), lit_type::primed);
        std::cout << fmt::format("{} + 1 = {}", i, sum) << std::endl;

        if (sum != i + 1)
          wrong++;
      }
      else
        wrong++;
    }

    std::cout << fmt::format("BitVec incrementation for i={}..{}", 0, max_value)
              << std::endl
              << fmt::format("{} wrong", wrong) << std::endl;

    wrong = 0;
    std::cout << "Brute" << std::endl;
    for (unsigned i = 0; i <= max_value; i++)
    {
      z3::solver s(ctx);

      s.add(bv.equals(i));
      expr_vector increment(ctx);
      for (BitVec::numrep_t x = 0; x <= max_value; x++)
      {
        expr set_index = implies(bv.equals(x), bv.p_equals(x + 1));
        // expr rest_stays =
        //     implies(!bv1.equals(x), bv1.unchanged()); // uhmm
        increment.push_back(set_index);
      }

      s.add(bv.incremented());

      std::optional<z3::expr_vector> result = z3ext::solver::check_witness(s);
      if (result)
      {
        unsigned sum = bv.extract_value(result.value(), lit_type::primed);
        std::cout << fmt::format("{} + 1 = {}", i, sum) << std::endl;

        if (sum != i + 1)
          wrong++;
      }
      else
        wrong++;
    }

    std::cout << fmt::format("BitVec incrementation for i={}..{}", 0, max_value)
              << std::endl
              << fmt::format("{} wrong", wrong) << std::endl;
  }

} // namespace mysat::primed
