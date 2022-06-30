#include <algorithm>
#include <fmt/color.h>
#include <fmt/core.h>
#include <z3++.h>
#include <z3_api.h>

#include "expr.h"
#include "peterson.h" // uncomment for coc syntax check

namespace peterson
{
  using z3::expr;
  using z3::expr_vector;
  using z3::forall;
  using z3::implies;
  using z3::mk_and;
  using z3::select;

  // forall expression with a single pattern for a single bound variable
  // the Z3_pattern is the only Z3 object created in this function, the rest
  // is taken from existing cpp-objects and does not need to be reference
  // counted explicitly
  inline expr forall_st(expr const& x, const expr& pattern, expr const& body)
  {
    check_context(x, body);
    check_context(pattern, body);

    unsigned num_bound = 1;
    Z3_app vars[]      = { (Z3_app)x };

    unsigned num_patt  = 1;
    Z3_ast patt_body[] = { (Z3_ast)pattern };
    Z3_pattern patt    = Z3_mk_pattern(body.ctx(), num_patt, patt_body);
    Z3_inc_ref(body.ctx(), Z3_pattern_to_ast(body.ctx(), patt));

    Z3_pattern patterns[] = { patt };

    Z3_ast r = Z3_mk_forall_const(
        body.ctx(), 0, num_bound, vars, num_patt, patterns, body);
    body.check_error();

    Z3_dec_ref(body.ctx(), Z3_pattern_to_ast(body.ctx(), patt));
    return expr(body.ctx(), r);
  }

  template <unsigned Bits> void Model<Bits>::bitvector_test(size_t max_value)
  {
    mysat::primed::BitVec bv(ctx, "b", Bits);
    unsigned wrong{ 0 };

    for (unsigned i = 2; i < max_value; i++)
    {
      for (unsigned j = std::pow(2, 4); j < max_value; j++)
      {
        assert(i < max_value && j < max_value);
        z3::solver s(ctx);

        s.add(bv.equals(i));
        s.add(bv.less<Bits>(j));
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
    std::cout << fmt::format("{} wrong comparisons", wrong) << std::endl;
    return;
  }

  template <unsigned Bits>
  Model<Bits>::Model(z3::config& settings, unsigned n_processes)
      : ctx(settings), N(n_processes), pc(), l(), last(ctx, "last", N, Bits),
        old_last(ctx), initial(ctx), transition(ctx), x(ctx), array_range(ctx)
  {
    using fmt::format;
    assert(N < INT_MAX);
    static_assert(Bits <= std::numeric_limits<unsigned>::digits);

    x                = ctx.int_const("x");
    array_range      = (x >= 0) & (x < (int)N);
    size_t max_state = 4;
    size_t pc_bits   = std::ceil(std::log2(max_state) + 1);
    size_t N_bits    = std::ceil(std::log2(N - 1) + 1);
    // 0 = idle, take to aquire lock
    // 1 = aquiring, take to bound check
    // 2 = aquiring, take to set last
    // 3 = aquiring, take to await
    // 4 = in critical section, take to release

    old_last = PrimedExpression::array(ctx, "old_last", ctx.int_sort());
    z3::expr A = ctx.bool_val(true);

    for (unsigned i = 0; i < N; i++)
    {
      std::string pc_i = format("pc_{}", i);
      pc.emplace_back(ctx, pc_i.c_str(), pc_bits);
      std::cout << pc[i]() << std::endl;

      std::string l_i = format("l_{}", i);
      l.emplace_back(ctx, l_i.c_str(), N_bits);

      std::string free_i = format("free_{}", i);
      free.emplace_back(ctx, free_i.c_str());

      initial.push_back(pc[i].equals(0));
      initial.push_back(l[i].equals(0));
      initial.push_back(free[i]);
      initial.push_back(last.store(i, 0));

      A = A && last.store(i, N-1-i);
    }

    std::cout << A << std::endl << "======" << std::endl;
    for (unsigned i = 0; i < N; i++)
      last.get_value(A, i);
    return;

    std::cout << initial << std::endl;
    return;
    // set all `old_last` elements to -1
    initial.push_back(forall(x, array_range & (select(old_last, x) == -1)));

    for (unsigned i = 0; i < N; i++)
    {
      transition.push_back(T_start(i));
      transition.push_back(T_boundcheckfail(i));
      transition.push_back(T_boundchecksucc(i));
      transition.push_back(T_setlast(i));
    }

    std::cout << "peterson transition" << std::endl;
    std::cout << transition << std::endl;
  }

  template <typename T,
      std::enable_if_t<std::is_base_of_v<mysat::primed::IStays, T>, bool> =
          true>
  void stays(z3::expr_vector& container, const std::vector<T>& v)
  {
    for (const auto& primed : v)
      container.push_back(primed.unchanged());
  }

  template <typename T,
      std::enable_if_t<std::is_base_of_v<mysat::primed::IStays, T>, bool> =
          true>
  void stays_except(
      z3::expr_vector& container, const std::vector<T>& v, size_t exception)
  {
    for (size_t i = 0; i < v.size(); i++)
      if (i != exception)
        container.push_back(v[i].unchanged());
  }

  template <unsigned Bits> expr Model<Bits>::T_start(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    // advance program counter
    conj.push_back(pc[i].equals(0));
    conj.push_back(pc[i].p_equals(1));

    // l[i] <- 0
    conj.push_back(free[i]);
    conj.push_back(l[i].p_equals(0));
    conj.push_back(!free[i].p());

    // all else stays
    stays_except(conj, pc, i);
    stays_except(conj, l, i);
    stays_except(conj, free, i);

    // for (unsigned j = 0; j)
    for (unsigned i = 0; i < last.size(); i++)
    {
      // if A{i] => x then A.p[i] => x
    }

    conj.push_back(forall(x,
        implies(array_range, select(old_last, x) == select(old_last.p(), x))));

    return z3::mk_and(conj);
  }

  template <unsigned Bits> expr Model<Bits>::T_boundcheckfail(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    conj.push_back(pc[i].equals(1));
    conj.push_back(pc[i].p_equals(4));

    conj.push_back(~l[i].less<Bits>(N)); // l[i] >= N

    // all else stays
    for (size_t j = 0; j < pc.size(); j++)
      if (j != i)
        conj.push_back(pc[i].unchanged());

    for (size_t j = 0; j < N; j++)
    {
      conj.push_back(l[i].unchanged());
      conj.push_back(free[i].unchanged());
    }
    conj.push_back( // TODO separate conjunction into multiple
        forall_st(
            x, array_range, select(old_last, x) == select(old_last.p(), x)));

    return z3::mk_and(conj);
  }

  template <unsigned Bits> expr Model<Bits>::T_boundchecksucc(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    conj.push_back(pc[i].equals(1));
    conj.push_back(pc[i].p_equals(2));

    conj.push_back(l[i].less<Bits>(N));

    // all else stays
    for (size_t j = 0; j < pc.size(); j++)
      if (j != i)
        conj.push_back(pc[i].unchanged());

    for (size_t j = 0; j < N; j++)
    {
      conj.push_back(l[i].unchanged());
      conj.push_back(free[i].unchanged());
    }
    conj.push_back(forall_st(
        x, array_range, select(old_last, x) == select(old_last.p(), x)));

    return z3::mk_and(conj);
  }

  template <unsigned Bits> expr Model<Bits>::T_setlast(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    conj.push_back(pc[i].equals(2));
    conj.push_back(pc[i].p_equals(3));

    // old_last[l[i]] <- i:
    // old_last[x] <- i, where x == l[i]
    // conj.push_back(x == l(i) && select(old_last.p(), x) == (int)i);

    // all else stays
    for (size_t j = 0; j < pc.size(); j++)
      if (j != i)
        conj.push_back(pc[i].unchanged());

    for (size_t j = 0; j < N; j++)
    {
      conj.push_back(l[i].unchanged());
      conj.push_back(free[i].unchanged());
    }
    // conj.push_back(forall_st(
    //     x, array_range && x != l(i), select(old_last, x) ==
    //     select(old_last.p(), x)));

    return z3::mk_and(conj);
  }

} // namespace peterson
