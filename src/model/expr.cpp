#include "expr.h"

namespace mysat::primed
{
  using z3::mk_and;
  using z3::expr;
  using z3::expr_vector;

  // Lit
  //
  Lit::Lit(z3::context& c, const std::string& name) : IPrimed<expr>(c)
  {
    std::string next_name = fmt::format("{}.p", name);
    current               = ctx.bool_const(name.c_str());
    next                  = ctx.bool_const(next_name.c_str());
  }

  Lit::operator const expr&() const { return current; }
  const expr& Lit::operator()() const { return current; }
  const expr& Lit::p() const { return next; }

  expr Lit::unchanged() const { return current == next; }

  // BitVec
  // public
  BitVec::BitVec(z3::context& c, const std::string& name, size_t Nbits)
      : IPrimed<expr_vector>(c), size(Nbits)
  {
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
  BitVec BitVec::holding(
      z3::context& c, const std::string& name, size_t max_val)
  {
    return BitVec(c, name, std::log2(max_val) + 1);
  }

  // return all literals comprising the vector
  BitVec::operator const expr_vector&() const { return current; }
  const expr_vector& BitVec::operator()() const { return current; }
  const expr_vector& BitVec::p() const { return next; }

  // access individual literals
  expr BitVec::operator()(size_t i) const { return current[i]; }
  expr BitVec::p(size_t i) const { return next[i]; }

  // bitvector to unsigned integer conversions
  expr_vector BitVec::uint(unsigned n) const
  {
    return unint_to_lits(n, false);
  }
  expr_vector BitVec::uint_p(unsigned n) const
  {
    return unint_to_lits(n, true);
  }

  // equality operator to expression conversions
  expr BitVec::equals(unsigned n) const { return mk_and(uint(n)); }
  expr BitVec::p_equals(unsigned n) const { return mk_and(uint_p(n)); }

  // all bits equal in current and next
  expr BitVec::unchanged() const
  {
    expr_vector conj(ctx);
    for (size_t i = 0; i < size; i++)
      conj.push_back(current[i] == next[i]);

    return mk_and(conj);
  }

  // private
  expr_vector BitVec::unint_to_lits(unsigned n, bool primed) const
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

  expr BitVec::less_4b(size_t i, unsigned n) const
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

  // Array
  // public
  Array::Array(z3::context& c, const std::string& name, unsigned s,
      unsigned bits)
      : _size(s), index(c, name + "_i", bits), value(c, name + "_v", bits), index_solver(c)
  {
  }

  unsigned Array::size() const { return _size; }

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

  expr_vector Array::get_value(const expr& A, unsigned i)
  {
    index_solver.reset();
    index_solver.add(A);
    z3::check_result r = index_solver.check(index.uint(i));

    assert(r == z3::check_result::sat);
    z3::model witness = index_solver.get_model();
    // TODO convert to ordered vector
    std::cout << witness << std::endl << "---" << std::endl;

    return expr_vector(index_solver.ctx());
  }
} // namespace mysat::primed
