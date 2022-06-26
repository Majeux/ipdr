#include "peterson.h"
#include <fmt/color.h>
#include <fmt/core.h>
#include <z3++.h>
#include <z3_api.h>

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

  Model::Model(z3::config& settings, unsigned n_processes)
      : ctx(settings), N(n_processes), pc(), l(), last(ctx), initial(ctx),
        transition(ctx), x(ctx), array_range(ctx)
  {
    using fmt::format;
    assert(N < INT_MAX);

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

    // mysat::primed::BitVec bv(ctx, "b", 7);
    // for (unsigned i = 2; i <= 7; i++)
    // {
    //   for (unsigned j = 0; j <= 7; j++)
    //   {
    //     z3::solver s(ctx);
    //     std::cout << fmt::format("{} < {}", i, j) << std::endl;

    //     s.add(bv.equals(i));
    //     s.add(bv.less(j));
    //     z3::check_result r = s.check();
    //     if (r == z3::check_result::sat)
    //       std::cout << "\tsat" << std::endl;
    //     if (r == z3::check_result::unsat)
    //       std::cout << "\tunsat" << std::endl;
    //   }
    // }
    // return;

    last = PrimedExpression::array(ctx, "last", ctx.int_sort());

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
    }
    std::cout << initial << std::endl;
    return;
    // set all `last` elements to -1
    initial.push_back(forall(x, array_range & (select(last, x) == -1)));

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

  void Model::stays(const PrimedExpressions& E, expr_vector& add_to)
  {
    for (size_t i = 0; i < E.size(); i++)
      add_to.push_back(E(i) == E.p(i));
  }

  void Model::stays_except(
      const PrimedExpressions& E, z3::expr_vector& add_to, size_t exception)
  {
    for (size_t i = 0; i < E.size(); i++)
    {
      if (i == exception)
        continue;
      add_to.push_back(E(i) == E.p(i));
    }
  }

  expr Model::T_start(unsigned i)
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
    for (size_t j = 0; j < pc.size(); j++)
      if (j != i)
        conj.push_back(pc[i].unchanged());

    for (size_t j = 0; j < N; j++)
    {
      if (j != i)
      {
        conj.push_back(l[i].unchanged());
        conj.push_back(free[i].unchanged());
      }
    }
    conj.push_back(forall(
        x, implies(array_range, select(last, x) == select(last.p(), x))));

    return z3::mk_and(conj);
  }

  expr Model::T_boundcheckfail(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    conj.push_back(pc[i].equals(1));
    conj.push_back(pc[i].p_equals(4));

    conj.push_back(!l[i].less(N)); // l[i] >= N

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
        forall_st(x, array_range, select(last, x) == select(last.p(), x)));

    return z3::mk_and(conj);
  }

  expr Model::T_boundchecksucc(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    conj.push_back(pc[i].equals(1));
    conj.push_back(pc[i].p_equals(2));

    conj.push_back(l[i].less(N));

    // all else stays
    for (size_t j = 0; j < pc.size(); j++)
      if (j != i)
        conj.push_back(pc[i].unchanged());

    for (size_t j = 0; j < N; j++)
    {
      conj.push_back(l[i].unchanged());
      conj.push_back(free[i].unchanged());
    }
    conj.push_back(
        forall_st(x, array_range, select(last, x) == select(last.p(), x)));

    return z3::mk_and(conj);
  }

  expr Model::T_setlast(unsigned i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    conj.push_back(pc[i].equals(2));
    conj.push_back(pc[i].p_equals(3));

    // last[l[i]] <- i:
    // last[x] <- i, where x == l[i]
    // conj.push_back(x == l(i) && select(last.p(), x) == (int)i);

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
    //     x, array_range && x != l(i), select(last, x) == select(last.p(), x)));

    return z3::mk_and(conj);
  }

} // namespace peterson
