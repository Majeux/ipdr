#include <algorithm>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <regex>
#include <stdexcept>
#include <tabulate/table.hpp>
#include <z3++.h>
#include <z3_api.h>

#include "expr.h"
#include "peterson.h"
#include "z3-ext.h"

namespace pdr::peterson
{
  using std::set;
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;
  using z3::implies;
  using z3::mk_and;

  // STATE MEMBERS
  //
  expr_vector PetersonState::cube(PetersonModel& m) const
  {
    using num_vec = std::vector<PetersonModel::numrep_t>;
    using bv_vec  = std::vector<PetersonModel::BitVec>;

    expr_vector conj(m.ctx);

    auto bv_assign = [&conj](const num_vec& nv, const bv_vec& bv)
    {
      assert(nv.size() == bv.size());
      for (size_t i = 0; i < nv.size(); i++)
        for (const expr& e : bv[i].uint(nv[i]))
          conj.push_back(e);
    };

    bv_assign(pc, m.pc);
    bv_assign(level, m.level);
    bv_assign(last, m.last);

    assert(free.size() == m.free.size());
    for (size_t i = 0; i < free.size(); i++)
      conj.push_back(free[i] ? m.free[i] : !m.free[i]());

    for (expr const& e : m.proc_last.uint(proc_last))
      conj.push_back(e);
    for (expr const& e : m.switch_count.uint(switch_count))
      conj.push_back(e);

    return conj;
  }

  bool PetersonState::operator<(const PetersonState& s) const
  {
    if (pc < s.pc)
      return true;
    if (s.pc < pc)
      return false;

    if (level < s.level)
      return true;
    if (s.level < level)
      return false;

    if (free < s.free)
      return true;
    if (s.free < free)
      return false;

    if (last < s.last)
      return true;
    if (s.last < last)
      return false;

    if (proc_last < s.proc_last)
      return true;
    if (s.proc_last < proc_last)
      return false;

    if (switch_count < s.switch_count)
      return true;
    if (s.switch_count < switch_count)
      return false;

    return false;
  }

  bool PetersonState::operator==(const PetersonState& s) const
  {
    if (pc != s.pc)
      return false;
    if (level != s.level)
      return false;
    if (free != s.free)
      return false;
    if (last != s.last)
      return false;
    if (proc_last != s.proc_last)
      return false;
    if (switch_count != s.switch_count)
      return false;

    return true;
  }

  bool PetersonState::operator!=(const PetersonState& s) const
  {
    return not(*this == s);
  }

  std::ostream& operator<<(std::ostream& out, const PetersonState& s)
  {
    out << s.to_string(false);
    return out;
  }

  string PetersonState::to_string(bool inl) const
  {
    using fmt::format;
    using std::endl;

    std::stringstream ss;
    auto end = [inl]() { return inl ? "" : "\n"; };
    auto tab = [inl](unsigned n) { return inl ? " " : string(2 * n, ' '); };

    ss << "State {" << end();
    {
      ss << tab(1) << "pc [" << end();
      for (PetersonModel::numrep_t i = 0; i < pc.size(); i++)
        ss << tab(2) << format("{},", pc.at(i)) << end();
      ss << tab(1) << "]," << end() << end();
    }
    {
      ss << tab(1) << "level [" << end();
      for (PetersonModel::numrep_t i = 0; i < level.size(); i++)
        ss << tab(2) << format("{},", level.at(i)) << end();
      ss << tab(1) << "]," << end() << end();
    }
    {
      ss << tab(1) << "free [" << end();
      for (PetersonModel::numrep_t i = 0; i < free.size(); i++)
        ss << tab(2) << format("{},", free.at(i) ? "t" : "f") << end();
      ss << tab(1) << "]," << end() << end();
    }
    {
      ss << tab(1) << "last [" << end();
      for (PetersonModel::numrep_t i = 0; i < last.size(); i++)
        ss << tab(2) << format("{},", last.at(i)) << end();
      ss << tab(1) << "]," << end();
    }
    ss << tab(1) << "proc_last = " << proc_last << "," << end();
    ss << tab(1) << "switch_count = " << switch_count << "," << end();

    ss << "}";

    return ss.str();
  }

  string PetersonState::inline_string() const { return to_string(true); }

  // PETERSONMODEL MEMBERS
  //
  size_t PetersonModel::n_lits() const
  {
    size_t total{ 0 };
    auto add_size = [](size_t acc, const BitVec& bv) -> size_t
    { return acc + bv.size; };

    total = std::accumulate(pc.begin(), pc.end(), total, add_size);
    total = std::accumulate(level.begin(), level.end(), total, add_size);
    total = std::accumulate(last.begin(), last.end(), total, add_size);
    total += free.size();
    total += proc.size;
    total += proc_last.size;
    total += switch_count.size;

    return total;
  }

  Vars PetersonModel::create_vars()
  {
    using fmt::format;
    using z3::atleast;
    using z3::atmost;

    // 0 = idle, take to aquire lock
    // 1 = aquiring, take to bound check
    // 2 = aquiring, take to set last
    // 3 = aquiring, take to await
    // 4 = in critical section, take to release (l[i] = N-1)

    for (numrep_t i = 0; i < N; i++)
    {
      {
        string pc_i = format("pc{}", i);
        pc.push_back(BitVec::holding(ctx, pc_i, pc_num));
      }
      {
        string l_i = format("level{}", i);
        level.push_back(BitVec::holding(ctx, l_i, N));
      }
      {
        string free_i = format("free{}", i);
        free.emplace_back(ctx, free_i);
      }
      if (i < N - 1)
      {
        string last_i = format("last{}", i);
        last.push_back(BitVec::holding(ctx, last_i, N));
      }
    }

    expr_vector conj(ctx), conj_p(ctx);
    {
      for (numrep_t i = 0; i < N; i++)
      {
#warning PETERSON PROPERTY: some redundant literals (pc and level). include or not??
        conj.push_back(
            pc.at(i).equals(4) && !free.at(i)() && level.at(i).equals(N - 1));

        conj_p.push_back(pc.at(i).p_equals(4) && !free.at(i).p() &&
                         level.at(i).p_equals(N - 1));
      }
    }
    property.add(atmost(conj, 1), atmost(conj_p, 1)).finish();
    n_property.add(atleast(conj, 2), atleast(conj_p, 2)).finish();
    // n_property.add(!atmost(conj, 1), !atmost(conj_p, 1)).finish();

    // collect symbol strings
    Vars rv;
    auto append_names = [&rv](const mysat::primed::INamed& v)
    {
      for (std::string_view n : v.names())
      {
        // std::cout << n << std::endl;
        rv.curr.emplace_back(n);
      }
      for (std::string_view n : v.names_p())
        rv.next.emplace_back(n);
    };

    for (const BitVec& var : pc)
      append_names(var);

    for (const BitVec& var : level)
      append_names(var);

    for (const Lit& var : free)
      append_names(var);

    for (const BitVec& var : last)
      append_names(var);

    return rv;
  }

  void PetersonModel::reset_initial()
  {
    initial.resize(0);

    // initialize vars at 0
    for (numrep_t i = 0; i < N; i++)
    {
      for (const expr& e : pc.at(i).uint(0))
        initial.push_back(e);

      for (const expr& e : level.at(i).uint(0))
        initial.push_back(e);

      initial.push_back(free.at(i));

      if (i < N - 1)
        for (const expr& e : last.at(i).uint(0))
          initial.push_back(e);
    }

    if (max_switches)
    {
      initial.push_back(proc_last.equals(0));
      initial.push_back(switch_count.equals(0));
    }
  }

  PetersonModel::PetersonModel(
      z3::context& c, numrep_t n_procs, numrep_t max_procs)
      : IModel(c, {}),
        N(max_procs),
        p(n_procs),
        proc(BitVec::holding(c, "proc", n_procs)),
        proc_last(BitVec::holding(c, "proc_last", n_procs)),
        switch_count(BitVec::holding(c, "n_switches", MAX_SWITCHES)),
        pc(),
        level(),
        last()
  {
    using fmt::format;
    Vars allvars = create_vars();
    vars.add(allvars.curr, allvars.next);

    for (auto n : vars.p())
      std::cout << n.to_string() << std::endl;

    assert(N < INT_MAX);

    reset_initial();
    constrain(n_procs);

    // test_bug();
    // test_p_pred();
    // test_property();
    // test_room();
    // bv_val_test(10);
    // bv_comp_test(10);
    // bv_inc_test(10);
  }

  const std::string PetersonModel::constraint_str() const
  {
    return fmt::format("{} active processes, out of {} max", p, N);
  }

  unsigned PetersonModel::constraint_num() const { return p; }

  unsigned PetersonModel::n_processes() const { return p; }
  unsigned PetersonModel::max_processes() const { return N; }

  expr if_then_else(const expr& i, const expr& t, const expr& e)
  {
    return (!i || t) && (i || e); // i => t || !i => e
  }

  void PetersonModel::constrain(numrep_t processes)
  {
    using mysat::primed::lit_type;
    using z3ext::tseytin::to_cnf;
    using z3ext::tseytin::to_cnf_vec;
    assert(processes <= N);

    {
      int d = processes - p;

      if (d > 0)
        diff = Diff_t::relaxed;
      else if (d < 0)
        diff = Diff_t::constrained;
      else // d == 0
        diff = Diff_t::none;
    }

    p = processes;
    transition.resize(0);
    constraint.resize(0);

    if (max_switches)
    {
      // store the pid that is used in this step
      //    proc_last' <- proc
      transition.push_back(proc_last.p_equals(proc).simplify());

      // count the number of times that the active process is switched
      //    if proc_last == proc then n_switches' <- n_switches
      //    else n_switches' <- n_switches + 1
      transition.push_back(
          to_cnf((proc_last.nequals(proc) || switch_count.unchanged()) &&
                 (proc_last.equals(proc) || switch_count.incremented()))
              .simplify());
      // transition.push_back(z3::ite(proc_last.equals(proc),
      // switch_count.unchanged(), switch_count.incremented()));

      // cannot take a transition that causes us to hit max_switches
      constraint.push_back(
          to_cnf(switch_count.less(*max_switches, lit_type::primed))
              .simplify());
    }

    // can be easily constrained be removing transitions for i >= p
    // leave transition empty, put decreasing T-relation in constraint
    expr_vector disj(ctx);
    for (numrep_t i = 0; i < p; i++)
    {
      expr i_steps = T_start(i) || T_boundcheck(i) || T_setlast(i) ||
                     T_await(i) || T_release(i);
      if (max_switches)
        disj.push_back(proc.equals(i) && i_steps);
      else
        disj.push_back(i_steps);
    }
    // transition = disj; // no
    // std::cout << "RAW TRANSITION" << std::endl;
    // std::cout << disj << std::endl;
    // constraint = to_cnf_vec(mk_or(disj));
    constraint.push_back(to_cnf(mk_or(disj)).simplify());
    // constraint.push_back(mk_or(disj);
    // std::cout << "CNF TRANSITION" << std::endl;
    // std::cout << transition << std::endl;
  }

  void PetersonModel::constrain_switches(numrep_t m)
  {
    if (m < MAX_SWITCHES) // cannot count past MAX_SWITCHES
    {
      max_switches = m;
      constrain(p);
    }
    else
    {
      throw std::invalid_argument(
          fmt::format("The maximum number of switches m (={}) cannot be higher "
                      "than MAX_SWITCHES={}",
              m, MAX_SWITCHES));
    }
  }

  template <typename T,
      std::enable_if_t<std::is_base_of_v<mysat::primed::IStays, T>, bool> =
          true>
  void stays(expr_vector& container, const vector<T>& v)
  {
    for (const auto& primed : v)
      container.push_back(primed.unchanged());
  }

  template <typename T,
      std::enable_if_t<std::is_base_of_v<mysat::primed::IStays, T>, bool> =
          true>
  void stays_except(
      expr_vector& container, const vector<T>& v, size_t exception)
  {
    for (size_t i = 0; i < v.size(); i++)
      if (i != exception)
        container.push_back(v.at(i).unchanged());
  }

  //  T
  //  0: idle
  //    -> 1. level[i] <- 0
  //  1: boundcheck
  //    -> if level[i] < N-1 then 2.
  //    -> if level[i] >= N-1 then 4.
  //  2: set last
  //    -> 3. last[level[i]] <- i
  //  3: wait
  //    -> if last[level[i]] == i && E k != i: level[k] >= level[i] then 3.
  //    -> else then 1. level[i] <- level[i] + 1
  //  4: critical section
  //    ->

  expr PetersonModel::T_start(numrep_t i)
  {
    assert(i < p);
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

  expr PetersonModel::T_boundcheck(numrep_t i)
  {
    assert(i < p);
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

  expr PetersonModel::T_setlast(numrep_t i)
  {
    assert(i < p);
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

  expr PetersonModel::T_await(numrep_t i)
  {
    assert(i < p);
    expr_vector conj(ctx);

    // pc[i] == 3
    conj.push_back(pc.at(i).equals(3));

    // IF last[i] == i AND EXISTS k != i: level[k] >= last[i]
    // THEN repeat 3
    // ELSE increment and go to loop bound
    expr branch(ctx);
    {
      expr check(ctx); // last[i] == i AND EXISTS k != i: level[k] >= last[i]
      {
        expr_vector eq_i(ctx); // last[l[i]] = i:
        for (numrep_t x = 0; x < N - 1; x++)
        {
          // if l[i] = x, we require last[x] = i
          eq_i.push_back(implies(level.at(i).equals(x), last.at(x).equals(i)));
        }

        expr_vector any_higher(ctx); // EXISTS k != i: level[k] >= last[i]
        for (numrep_t k = 0; k < N; k++)
        {
          if (k != i)             // forall k != i:
            any_higher.push_back( // l[i] < l[k], free acts as a sign bit
                !free.at(k)() &&
                (free.at(i) ||
                    !level.at(k).less(level.at(i)))); // if free[i], l[i]=-1
        }
        check = mk_and(eq_i) && mk_or(any_higher);
      }

      // l[i]++
      expr incremented(ctx);
      {
        expr_vector increment(ctx);
        for (numrep_t x = 0; x < N - 1; x++)
        {
          expr set_index =
              implies(level.at(i).equals(x), level.at(i).p_equals(x + 1));
          // expr rest_stays =
          //     implies(!level.at(i).equals(x), level.at(i).unchanged()); //
          //     uhmm
          increment.push_back(set_index);
        }
        expr raw   = z3ext::tseytin::to_cnf(z3::mk_and(increment));
        expr adder = z3ext::tseytin::to_cnf(level.at(i).incremented());

        // incremented = raw.num_args() < adder.num_args() ? raw : adder;
        incremented = adder;
      }

      expr wait     = pc.at(i).p_equals(3) && level.at(i).unchanged();
      expr end_loop = pc.at(i).p_equals(1) && incremented;

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

  expr PetersonModel::T_release(numrep_t i)
  {
    assert(i < p);
    expr_vector conj(ctx);

    // pc[i] == 4
    conj.push_back(pc.at(i).equals(4));
    conj.push_back(level.at(i).equals(N - 1)); // not needed?
    conj.push_back(level.at(i).p_equals(0));   // not needed?

    // pc[i] <- 0
    conj.push_back(pc.at(i).p_equals(0));
    // release lock
    conj.push_back(!free.at(i)());
    conj.push_back(free.at(i).p());

    // all else stays
    stays_except(conj, pc, i); // pc[i] stays or changes based on logic above
    stays_except(conj, level, i);
    stays_except(conj, free, i);
    stays(conj, last);

    return mk_and(conj);
  }

  // testing functions
  //
  void PetersonModel::test_p_pred()
  {
    z3::solver solver(ctx);
    solver.set("sat.cardinality.solver", true);
    solver.set("cardinality.solver", true);
    solver.add(property);
    solver.add(constraint);

    z3::check_result result = solver.check(n_property.p());

    if (result == z3::check_result::sat)
      std::cout << solver.get_model() << std::endl;
    else
      std::cout << "unsat" << std::endl;
  }

  void PetersonModel::test_bug()
  {
    z3::solver solver(ctx);
    solver.set("sat.cardinality.solver", true);
    solver.set("cardinality.solver", true);

    // solver.add(property);
    // std::cout << "property";

    solver.add(n_property);
    std::cout << "n_property";

    std::cout << solver << std::endl;
    std::cout << std::endl;

    expr_vector final(ctx), cti(ctx);
    {
      final.push_back(!free.at(0)());
      final.push_back(!free.at(1)());
      final.push_back(!free.at(2)());
    }

    size_t Nlits = n_lits();

    if (z3::check_result r = solver.check(final))
    {
      auto witness = z3ext::solver::get_witness(solver);
      std::cout << " - final: sat" << std::endl
                << fmt::format("witness ({}/{}): {}", witness.size(), Nlits,
                       witness.to_string())
                << std::endl
                << extract_state(witness).to_string(true) << std::endl;
    }
    else
    {
      auto core = z3ext::solver::get_core(solver);
      std::cout << " - final: unsat" << std::endl
                << fmt::format(
                       "core ({}/{}): {}", core.size(), Nlits, core.to_string())
                << extract_state(core).to_string(true) << std::endl;
    }
  }

  void PetersonModel::test_property()
  {
    z3::solver solver(ctx);
    solver.set("sat.cardinality.solver", true);
    solver.set("cardinality.solver", true);
    solver.add(property);
    std::cout << "property" << std::endl;
    // std::cout << property() << std::endl;
    // std::cout << property.p() << std::endl;
    std::cout << solver << std::endl;
    std::cout << std::endl;

    expr_vector no_crit(ctx), one_crit(ctx), two_crit(ctx), three_crit(ctx),
        four_crit(ctx);
    {
      no_crit.push_back(level.at(0).equals(0));
      no_crit.push_back(level.at(1).equals(0));
      no_crit.push_back(level.at(2).equals(0));
      no_crit.push_back(level.at(3).equals(0));
    }
    {
      one_crit.push_back(level.at(0).equals(0));
      one_crit.push_back(level.at(1).equals(N - 1));
      one_crit.push_back(level.at(2).equals(0));
      one_crit.push_back(level.at(3).equals(0));
    }
    {
      two_crit.push_back(level.at(0).equals(0));
      two_crit.push_back(level.at(1).equals(N - 1));
      two_crit.push_back(level.at(2).equals(N - 1));
      two_crit.push_back(level.at(3).equals(0));
    }
    {
      three_crit.push_back(level.at(0).equals(N - 1));
      three_crit.push_back(level.at(1).equals(0));
      three_crit.push_back(level.at(2).equals(N - 1));
      three_crit.push_back(level.at(3).equals(N - 1));
    }
    {
      four_crit.push_back(level.at(0).equals(N - 1));
      four_crit.push_back(level.at(1).equals(N - 1));
      four_crit.push_back(level.at(2).equals(N - 1));
      four_crit.push_back(level.at(3).equals(N - 1));
    }

    if (z3::check_result r = solver.check(no_crit))
      std::cout << "property - no_crit: sat" << std::endl;
    else
      std::cout << "property - no_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(one_crit))
      std::cout << "property - one_crit: sat" << std::endl;
    else
      std::cout << "property - one_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(two_crit))
      std::cout << "property - two_crit: sat" << std::endl;
    else
      std::cout << "property - two_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(three_crit))
      std::cout << "property - three_crit: sat" << std::endl;
    else
      std::cout << "property - three_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(four_crit))
      std::cout << "property - four_crit: sat" << std::endl;
    else
      std::cout << "property - four_crit: unsat" << std::endl;

    std::cout << std::endl;
    std::cout << std::endl;

    solver.reset();
    solver.add(n_property);
    std::cout << "n_property" << std::endl;
    // std::cout << n_property() << std::endl;
    // std::cout << n_property.p() << std::endl;
    std::cout << solver << std::endl;
    std::cout << std::endl;

    if (z3::check_result r = solver.check(no_crit))
      std::cout << "n_property - no_crit: sat" << std::endl;
    else
      std::cout << "n_property - no_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(one_crit))
      std::cout << "n_property - one_crit: sat" << std::endl;
    else
      std::cout << "n_property - one_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(two_crit))
      std::cout << "n_property - two_crit: sat" << std::endl;
    else
      std::cout << "n_property - two_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(three_crit))
      std::cout << "n_property - three_crit: sat" << std::endl;
    else
      std::cout << "n_property - three_crit: unsat" << std::endl;

    if (z3::check_result r = solver.check(four_crit))
      std::cout << "n_property - four_crit: sat" << std::endl;
    else
      std::cout << "n_property - four_crit: unsat" << std::endl;
  }

  void PetersonModel::test_room()
  {
    using mysat::primed::lit_type;
    using std::cout;
    using std::endl;
    using std::map;
    using std::queue;
    using std::set;

    std::ofstream out("peter-out.txt");
    // std::ostream& out = cout;
    cout << "n procs = " << p << std::endl;

    queue<PetersonState> Q;
    set<PetersonState> visited;
    map<PetersonState, set<PetersonState>> edges;
    // create a set if none exists, and insert destination
    auto edges_insert = [&edges](
                            const PetersonState& src, const PetersonState& dst)
    {
      set<PetersonState> s;
      set<PetersonState>& dst_set = edges.emplace(src, s).first->second;
      dst_set.insert(dst);
    };

    const PetersonState I = extract_state(initial);
    Q.push(I);

    while (not Q.empty())
    {
      const PetersonState& source = Q.front();
      if (visited.insert(source).second) // if source was not done already
      {
        for (const PetersonState& dest : successors(source))
        {
          if (visited.find(dest) == visited.end())
            Q.push(dest);
          edges_insert(source, dest);
        }
      }
      Q.pop();
    }

    unsigned size = std::accumulate(edges.begin(), edges.end(), 0,
        [](unsigned a, const auto& s) { return a + s.second.size(); });

    cout << fmt::format("No. edges = {}", size) << endl;

    out << "digraph G {" << endl;
    out << fmt::format("start -> \"{}\"", I.inline_string()) << endl;
    for (const auto& map_pair : edges)
    {
      assert(map_pair.second.size() <= p);

      string src_str = map_pair.first.inline_string();

      for (const PetersonState& dst : map_pair.second)
        out << fmt::format("\"{}\" -> \"{}\"", src_str, dst.inline_string())
            << endl
            << endl;
    }
    out << "}" << endl;
  }

  // PetersonState members
  //
  PetersonState PetersonModel::extract_state(
      const expr_vector& cube, mysat::primed::lit_type t) const
  {
    PetersonState s(N);

    for (numrep_t i = 0; i < N; i++)
    {
      s.pc.at(i)    = pc.at(i).extract_value(cube, t);
      s.level.at(i) = level.at(i).extract_value(cube, t);
      s.free.at(i)  = free.at(i).extract_value(cube, t);
      if (i < s.last.size())
        s.last.at(i) = last.at(i).extract_value(cube, t);
    }
    s.proc_last    = proc_last.extract_value(cube, t);
    s.switch_count = proc_last.extract_value(cube, t);

    return s;
  }

  PetersonState PetersonModel::extract_state_p(const expr_vector& cube) const
  {
    return extract_state(cube, mysat::primed::lit_type::primed);
  }

  set<PetersonState> PetersonModel::successors(const expr_vector& v)
  {
    return successors(extract_state(v));
  }

  set<PetersonState> PetersonModel::successors(const PetersonState& s)
  {
    using mysat::primed::lit_type;
    using std::optional;
    using z3ext::solver::check_witness;

    set<PetersonState> S;

    z3::solver solver(ctx);
    solver.add(s.cube(*this));
    solver.add(transition);
    solver.add(constraint);

    while (optional<expr_vector> w = check_witness(solver))
    {
      S.insert(extract_state(*w, lit_type::primed));
      solver.add(!mk_and(*w)); // exclude from future search
    }

    return S;
  }
} // namespace pdr::peterson
