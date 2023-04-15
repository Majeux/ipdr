#include "expr.h"
#include "z3-ext.h"
#include <cassert>
#include <cstdlib>
#include <fmt/core.h>
#include <optional>
#include <regex>
#include <stdexcept>

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

  bool is_reserved_lit(std::string_view name)
  {
    return name.size() >= 2 && name[0] == '_' && name[1] == '_';
  }

  bool is_reserved_lit(expr const& e) { return is_reserved_lit(e.to_string()); }

  void validate_lit_name(std::string_view name)
  {
    if (is_reserved_lit(name))
      throw ReservedLiteral(name);
  }

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
    validate_lit_name(n);
    current = ctx.bool_const(name.c_str());
    next    = ctx.bool_const(next_name.c_str());
  }

  Lit::operator const expr&() const { return current; }
  const expr& Lit::operator()() const { return current; }
  const expr& Lit::p() const { return next; }

  expr Lit::unchanged() const { return current == next; }

  vector<string> Lit::names() const { return { current.to_string() }; }
  vector<string> Lit::names_p() const { return { next.to_string() }; }

  bool Lit::extract_value(const z3::expr_vector& cube, lit_type t) const
  {
    // matches name[i], extracts name and i
    std::regex is_value;
    string match_name;
    if (t == primed)
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

    return v.value(); // no value if no literal was found
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
      validate_lit_name(n);
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
      validate_lit_name(currnames[i]);
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
      if (is_reserved_lit(var))
        return e;
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

#warning current/next literal checks does not work with constraint lits?
  bool VarVec::lit_is_current(const z3::expr& e) const
  {
    expr key = strip_not(e);
    if (is_reserved_lit(key))
      return true;
    return to_next.find(key.id()) != to_next.end();
  }

  bool VarVec::lit_is_p(const z3::expr& e) const
  {
    bool rv{ false };
    try
    {
      expr key = strip_not(e);
      if (is_reserved_lit(key))
        return true;
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
  } // namespace

  BitVec::BitVec(z3::context& c, const string& n, size_t Nbits)
      : IPrimed<expr_vector>(c, n), size(Nbits)
  {
    validate_lit_name(n);
    for (size_t i = 0; i < size; i++)
    {
      current.push_back(ctx.bool_const(index_str(name, i).c_str()));
      next.push_back(ctx.bool_const(index_str(next_name, i).c_str()));
    }
  }

  BitVec BitVec::holding(z3::context& c, const string& n, numrep_t max_val)
  {
    return BitVec(c, n, std::log2(max_val) + 1);
  }

  BitVec::operator const expr_vector&() const { return current; }
  const expr_vector& BitVec::operator()() const { return current; }
  const expr_vector& BitVec::p() const { return next; }

  expr BitVec::operator()(size_t i) const { return current[i]; }
  expr BitVec::p(size_t i) const { return next[i]; }

  vector<string> BitVec::names() const { return extract_names(current); }
  vector<string> BitVec::names_p() const { return extract_names(next); }

  expr_vector BitVec::uint(numrep_t n) const { return unint_to_lits(n, false); }
  expr_vector BitVec::uint_p(numrep_t n) const
  {
    return unint_to_lits(n, true);
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
    const string match_name{ t == primed ? next_name : name };
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

  expr BitVec::less_4b(numrep_t n, size_t msb) const
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
      expr bit = i < (int)size ? current[i] : ctx.bool_val(false);
      expr e   = !bit && set(i);
      for (size_t j = i + 1; j <= msb; j++)
      {
        // handle if BitVec does not store i+1 bits
        expr bit = j < size ? current[j] : ctx.bool_val(false);
        e        = !(bit ^ set(j)) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  expr BitVec::less_4b(const BitVec& cube, size_t msb) const
  {
    assert(size == cube.size);
    assert(msb < INT_MAX);
    assert((msb + 1) % 4 == 0);
    size_t lsb = msb - 3;

    expr_vector disj(ctx);
    for (int i = msb; i >= (int)lsb; i--)
    {
      expr bit      = i < (int)size ? current[i] : ctx.bool_val(false);
      expr cube_bit = i < (int)size ? cube(i) : ctx.bool_val(false);
      expr e        = !bit && cube_bit;
      for (size_t j = i + 1; j <= msb; j++)
      {
        // handle if BitVec does not store i+1 bits
        expr bit      = j < size ? current[j] : ctx.bool_val(false);
        expr cube_bit = j < size ? cube(j) : ctx.bool_val(false);
        e             = !(bit ^ cube_bit) && e;
      }
      disj.push_back(e);
    }

    return z3::mk_or(disj);
  }

  z3::expr BitVec::eq(numrep_t n, size_t msb, size_t nbits) const
  {
    assert((msb + 1) % 4 == 0);
    assert((msb + 1) >= nbits);
    size_t lsb = msb - (nbits - 1);

    const std::bitset<MAX_BITS> bits(n);

    z3::expr_vector conj(ctx);
    for (size_t i = lsb; i <= msb; i++)
    {
      expr bit = i < size ? current[i] : ctx.bool_val(false);
      if (bits.test(i))
        conj.push_back(bit);
      else
        conj.push_back(!bit);
    }

    return z3::mk_and(conj);
  }

  z3::expr BitVec::eq(const BitVec& cube, size_t msb, size_t nbits) const
  {
    assert(size == cube.size);
    assert((msb + 1) % 4 == 0);
    assert((msb + 1) >= nbits);
    size_t lsb = msb - (nbits - 1);

    z3::expr_vector conj(ctx);
    for (size_t i = lsb; i <= msb; i++)
    {
      expr bit      = i < size ? current[i] : ctx.bool_val(false);
      expr cube_bit = i < size ? cube(i) : ctx.bool_val(false);
      conj.push_back(!(cube_bit ^ bit)); // n[i] <=> current[i]
    }

    return z3::mk_and(conj);
  }
} // namespace mysat::primed
