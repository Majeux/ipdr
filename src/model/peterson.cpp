#include <algorithm>
#include <fmt/color.h>
#include <fmt/core.h>
#include <z3++.h>
#include <z3_api.h>

#include "expr.h"
#include "peterson.h"

namespace peterson
{
  using z3::expr;
  using z3::expr_vector;
  using z3::implies;
  using z3::mk_and;

  // STATE MEMBERS
  //
  std::ostream& operator<<(std::ostream& out, const Model::State& s)
  {
    using fmt::format;
    using std::endl;

    auto tab = [](unsigned n) { return std::string(2 * n, ' '); };
    out << "State {" << endl;
    {
      out << tab(1) << "pc [" << endl;
      for (Model::numrep_t i = 0; i < s.pc.size(); i++)
        out << tab(2) << format("{}: {},", i, s.pc.at(i)) << endl;
      out << tab(1) << "]" << endl << endl;
    }
    {
      out << tab(1) << "level [" << endl;
      for (Model::numrep_t i = 0; i < s.level.size(); i++)
        out << tab(2) << format("{}: {},", i, s.level.at(i)) << endl;
      out << tab(1) << "]" << endl << endl;
    }
    {
      out << tab(1) << "free [" << endl;
      for (Model::numrep_t i = 0; i < s.free.size(); i++)
        out << tab(2) << format("{}: {},", i, s.free.at(i) ? "true" : "false")
            << endl;
      out << tab(1) << "]" << endl << endl;
    }
    {
      out << tab(1) << "last [" << endl;
      for (Model::numrep_t i = 0; i < s.last.size(); i++)
        out << tab(2) << format("{}: {},", i, s.last.at(i)) << endl;
      out << tab(1) << "]" << endl;
    }
    out << "}" << endl;

    return out;
  }

  // MODEL MEMBERS
  //
  Model::Model(size_t bits, z3::config& settings, numrep_t n_processes)
      : ctx(settings), nbits(bits), N(n_processes), pc(), level(), last(),
        initial(ctx), transition(ctx)
  {
    using fmt::format;
    assert(N < INT_MAX);
    assert(nbits <= std::numeric_limits<numrep_t>::digits);

    size_t max_state = 4;
    size_t pc_bits   = std::ceil(std::log2(max_state) + 1);
    size_t N_bits    = std::ceil(std::log2(N - 1) + 1);
    // 0 = idle, take to aquire lock
    // 1 = aquiring, take to bound check
    // 2 = aquiring, take to set last
    // 3 = aquiring, take to await
    // 4 = in critical section, take to release (l[i] = N-1)

    for (numrep_t i = 0; i < N; i++)
    {
      {
        std::string pc_i = format("pc_{}", i);
        pc.emplace_back(ctx, pc_i, pc_bits);
      }
      {
        std::string l_i = format("l_{}", i);
        level.emplace_back(ctx, l_i, N_bits);
      }
      {
        std::string free_i = format("free_{}", i);
        free.emplace_back(ctx, free_i);
      }
      if (i < N - 1)
      {
        std::string last_i = format("last_{}", i);
        last.emplace_back(ctx, last_i, N_bits);
      }

      initial.push_back(pc.at(i).equals(0));
      initial.push_back(level.at(i).equals(0));
      initial.push_back(free.at(i));
      if (i < N - 1)
        initial.push_back(last.at(i).equals(0));
    }

    std::cout << "INITIAL" << std::endl;
    std::cout << initial << std::endl << std::endl;

    {
      expr_vector disj(ctx);
      for (numrep_t i = 0; i < N; i++)
      {
        disj.push_back(T_start(i));
        disj.push_back(T_boundcheck(i));
        disj.push_back(T_setlast(i));
        disj.push_back(T_await(i));
        disj.push_back(T_release(i));
      }
      // transition = disj; // no
      // std::cout << "RAW TRANSITION" << std::endl;
      // std::cout << disj << std::endl;
      transition = z3ext::tseytin::to_cnf_vec(mk_or(disj));
      // std::cout << "CNF TRANSITION" << std::endl;
      // std::cout << transition << std::endl;
    }
    test_room();
  }

  Model::State Model::make_state(const z3::expr_vector& cube, mysat::primed::lit_type t)
  {
    State s(N);
    std::cout << cube << std::endl;

    for (numrep_t i = 0; i < N; i++)
    {
      s.pc.at(i)    = pc.at(i).extract_value(cube, t);
      s.level.at(i) = level.at(i).extract_value(cube, t);
      s.free.at(i)  = free.at(i).extract_value(cube, t);
      if (i < s.last.size())
        s.last.at(i) = last.at(i).extract_value(cube, t);
    }

    return s;
  }

  void Model::test_room()
  {
    using mysat::primed::lit_type;

    z3::solver solver(ctx);
    solver.add(initial);

    z3::expr_vector i_witness = z3ext::solver::check_witness(solver).value();
    z3::expr_vector t_witness =
        z3ext::solver::check_witness(solver, transition).value();

    std::cout << i_witness << std::endl;
    std::cout << t_witness << std::endl;

    std::cout << make_state(i_witness) << std::endl;
    std::cout << make_state(t_witness, lit_type::primed) << std::endl;
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
        container.push_back(v.at(i).unchanged());
  }

  void array_stays(z3::expr_vector& container, const Model::Array& A)
  {
    for (unsigned i = 0; i < A.size; i++)
      for (Model::Array::numrep_t v = 0; v < A.n_vals; v++)
      {
        // (A{i] == v) => (A.p[i] <- v.p)
        z3::expr e = z3::implies(A.contains(i, v), A.store_p(i, v));
        container.push_back(e);
      }
  }

  void array_stays_except(
      z3::expr_vector& container, const Model::Array& A, size_t exception)
  {
    for (unsigned i = 0; i < A.size; i++)
      for (Model::Array::numrep_t v = 0; v < A.n_vals; v++)
      {
        if (i == exception)
          continue;
        // (A{i] == v) => (A.p[i] <- v.p)
        z3::expr e = z3::implies(A.contains(i, v), A.store_p(i, v));
        container.push_back(e);
      }
  }

  expr Model::T_start(numrep_t i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    // pc[i] == 0
    conj.push_back(pc.at(i).equals(0));
    // pc[i].p <- 1
    conj.push_back(pc.at(i).p_equals(1));

    // l[i] was released, but now enters the queue
    conj.push_back(free.at(i));
    conj.push_back(!free.at(i).p());
    // l[i] <- 0
    conj.push_back(level.at(i).p_equals(0));

    // all else stays
    stays_except(conj, pc, i);
    stays_except(conj, level, i);
    stays_except(conj, free, i);
    stays(conj, last);

    return z3::mk_and(conj);
  }

  expr if_then_else(const expr& i, const expr& t, const expr& e)
  {
    return implies(i, t) && implies(!i, e);
  }

  expr Model::T_boundcheck(numrep_t i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    // pc[i] == 1
    conj.push_back(pc.at(i).equals(1));

    // IF l[i] < N-1
    // THEN pc[i].p <- 2
    // ELSE pc[i].p <- 4
    conj.push_back(if_then_else(
        level.at(i).less(N - 1), pc.at(i).p_equals(2), pc.at(i).p_equals(4)));

    // all else stays
    stays_except(conj, pc, i);
    stays(conj, level);
    stays(conj, free);
    stays(conj, last);

    return z3::mk_and(conj);
  }

  expr Model::T_setlast(numrep_t i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    // pc[i] == 2
    conj.push_back(pc.at(i).equals(2));
    // pc[i].p <- 3
    conj.push_back(pc.at(i).p_equals(3));

    // old_last[l[i]] <- i:
    for (numrep_t x = 0; x < N - 1; x++)
    {
      expr branch = if_then_else(level.at(i).equals(x), last.at(x).p_equals(i),
          last.at(x).unchanged());
      conj.push_back(branch);
    }

    // all else stays
    stays_except(conj, pc, i);
    stays(conj, level);
    stays(conj, free);

    return z3::mk_and(conj);
  }

  expr Model::T_await(numrep_t i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    // pc[i] == 3
    conj.push_back(pc.at(i).equals(3));

    // IF last[i] == i AND EXISTS k != i: level[k] >= last[i]
    // THEN repeat 3
    // ELSE increment and go to loop bound
    expr branch(ctx);
    {
      expr_vector any_higher(ctx);
      {
        for (numrep_t k = 0; k < N; k++)
          if (k != i)             // forall k != i:
            any_higher.push_back( // l[i] < l[k]
                !free.at(k)() &&
                (free.at(i) ||
                    level.at(i).less(level.at(k)))); // if free[i], l[i]=-1
      }
      // last[l[i]] = i:
      expr_vector eq_i(ctx);
      for (numrep_t x = 0; x < N - 1; x++)
      {
        // if l[i] = x, we require last[x] = i
        expr branch = implies(level.at(i).equals(x), last.at(x).equals(i));
        eq_i.push_back(branch);
      }
      expr check = mk_or(eq_i) && mk_or(any_higher);

      // l[i]++
      expr_vector increment(ctx);
      for (numrep_t x = 0; x < N - 1; x++)
      {
        expr set_index =
            implies(level.at(i).equals(x), level.at(i).p_equals(x + 1));
        expr rest_stays =
            implies(!level.at(i).equals(x), level.at(i).unchanged());
        increment.push_back(set_index && rest_stays);
      }

      expr wait     = pc.at(i).p_equals(3) && level.at(i).unchanged();
      expr end_loop = pc.at(i).p_equals(1) && mk_and(increment);

      branch = if_then_else(check, wait, end_loop);
    }
    conj.push_back(branch);

    // all else stays
    stays_except(conj, pc, i); // pc[i] stays or changes based on logic above
    stays_except(conj, level, i);
    stays(conj, free);
    stays(conj, last);

    return mk_and(conj);
  }

  expr Model::T_release(numrep_t i)
  {
    assert(i < N);
    expr_vector conj(ctx);

    // pc[i] == 4
    conj.push_back(pc.at(i).equals(4));
    conj.push_back(level.at(i).equals(N - 1)); // not needed?

    // pc[i] <- 0
    conj.push_back(pc.at(i).equals(0));
    // release lock
    conj.push_back(!free.at(i)());
    conj.push_back(free.at(i).p());

    return mk_and(conj);
  }

  void Model::bitvector_test(size_t max_value)
  {
    mysat::primed::BitVec bv(ctx, "b", nbits);
    unsigned wrong{ 0 };

    for (unsigned i = 2; i < max_value; i++)
    {
      for (unsigned j = std::pow(2, 4); j < max_value; j++)
      {
        assert(i < max_value && j < max_value);
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
    std::cout << fmt::format("{} wrong comparisons", wrong) << std::endl;
    return;
  }
} // namespace peterson
