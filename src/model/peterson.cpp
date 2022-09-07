#include <algorithm>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fstream>
#include <map>
#include <numeric>
#include <queue>
#include <regex>
#include <tabulate/table.hpp>
#include <z3++.h>
#include <z3_api.h>

#include "expr.h"
#include "peterson.h"

namespace peterson
{
  using std::string;
  using std::vector;
  using std::set;
  using z3::expr;
  using z3::expr_vector;
  using z3::implies;
  using z3::mk_and;

  // STATE MEMBERS
  //
  bool State::operator<(const State& s) const
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

    // return inline_string() < s.inline_string();
    return false;
  }

  bool State::operator==(const State& s) const
  {
    if (pc != s.pc)
      return false;
    if (level != s.level)
      return false;
    if (free != s.free)
      return false;
    if (last != s.last)
      return false;

    return true;
  }

  bool State::operator!=(const State& s) const { return not(*this == s); }

  expr_vector State::cube(Model& m) const
  {
    using num_vec = std::vector<Model::numrep_t>;
    using bv_vec  = std::vector<Model::BitVec>;

    expr_vector conj(m.ctx);

    auto bv_assign = [&conj](const num_vec& N, const bv_vec& B)
    {
      assert(N.size() == B.size());
      for (size_t i = 0; i < N.size(); i++)
        for (const expr& e : B[i].uint(N[i]))
          conj.push_back(e);
    };

    bv_assign(pc, m.pc);
    bv_assign(level, m.level);
    bv_assign(last, m.last);

    assert(free.size() == m.free.size());
    for (size_t i = 0; i < free.size(); i++)
      conj.push_back(free[i] ? m.free[i] : !m.free[i]());

    return conj;
  }

  std::ostream& operator<<(std::ostream& out, const State& s)
  {
    out << s.to_string(false);
    return out;
  }

  string State::to_string(bool inl) const
  {
    using fmt::format;
    using std::endl;

    std::stringstream ss;
    auto end = [inl]() { return inl ? "" : "\n"; };
    auto tab = [inl](unsigned n) { return inl ? " " : string(2 * n, ' '); };

    ss << "State {" << end();
    {
      ss << tab(1) << "pc [" << end();
      for (Model::numrep_t i = 0; i < pc.size(); i++)
        ss << tab(2) << format("{},", pc.at(i)) << end();
      ss << tab(1) << "]" << end() << end();
    }
    {
      ss << tab(1) << "level [" << end();
      for (Model::numrep_t i = 0; i < level.size(); i++)
        ss << tab(2) << format("{},", level.at(i)) << end();
      ss << tab(1) << "]" << end() << end();
    }
    {
      ss << tab(1) << "free [" << end();
      for (Model::numrep_t i = 0; i < free.size(); i++)
        ss << tab(2) << format("{},", free.at(i) ? "true" : "false") << end();
      ss << tab(1) << "]" << end() << end();
    }
    {
      ss << tab(1) << "last [" << end();
      for (Model::numrep_t i = 0; i < last.size(); i++)
        ss << tab(2) << format("{},", last.at(i)) << end();
      ss << tab(1) << "]" << end();
    }
    ss << "}";

    return ss.str();
  }

  string State::inline_string() const { return to_string(true); }

  // MODEL MEMBERS
  //
  size_t bits_for(Model::numrep_t n)
  {
    size_t bits = std::ceil(std::log2(n - 1) + 1);
    assert(bits <= std::numeric_limits<Model::numrep_t>::digits);
    return bits;
  }

  set<string> Model::create_vars()
  {
    using fmt::format;

    size_t pc_bits = bits_for(pc_num);
    size_t N_bits  = bits_for(N);
    // 0 = idle, take to aquire lock
    // 1 = aquiring, take to bound check
    // 2 = aquiring, take to set last
    // 3 = aquiring, take to await
    // 4 = in critical section, take to release (l[i] = N-1)

    for (numrep_t i = 0; i < N; i++)
    {
      {
        string pc_i = format("pc_{}", i);
        pc.emplace_back(ctx, pc_i, pc_bits);
      }
      {
        string l_i = format("l_{}", i);
        level.emplace_back(ctx, l_i, N_bits);
      }
      {
        string free_i = format("free_{}", i);
        free.emplace_back(ctx, free_i);
      }
      if (i < N - 1)
      {
        string last_i = format("last_{}", i);
        last.emplace_back(ctx, last_i, N_bits);
      }
    }

    expr_vector conj(ctx), critical(ctx);
    {
      for (numrep_t i = 0; i < N; i++)
      {
        expr crit_i = ctx.bool_const(format("crit_{}", i).c_str());
        critical.push_back(crit_i); // add to rv

        conj.push_back(implies(level.at(i).equals(N - 1), crit_i));
      }
    }
    conj = z3ext::tseytin::to_cnf_vec(mk_and(conj)); // TODO add to all queries
    property.add(z3::atmost(critical, 1)).finish();
    n_property.add(z3::atleast(critical, 2)).finish(); 

    set<string> rv;
    auto append_names = [&rv](const mysat::primed::INamed& v)
    {
      for (std::string_view n : v.names())
        rv.emplace(n);
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

  Model::Model(z3::context& c,  numrep_t n_processes)
      : IModel(c, create_vars()), N(n_processes), pc(), level(), last()
  {
    using fmt::format;
    using z3ext::tseytin::to_cnf_vec;

    assert(N < INT_MAX);

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

    std::cout << "INITIAL" << std::endl;
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
      transition = to_cnf_vec(mk_or(disj));
      // std::cout << "CNF TRANSITION" << std::endl;
      // std::cout << transition << std::endl;
    }

    test_room();
    // bv_val_test(10);
    // bv_comp_test(10);
  }

  z3::expr_vector constraint(std::optional<unsigned> x) {}

  State Model::extract_state(const expr_vector& cube, mysat::primed::lit_type t)
  {
    State s(N);

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

  State Model::extract_state_p(const expr_vector& cube)
  {
    return extract_state(cube, mysat::primed::lit_type::primed);
  }

  set<State> Model::successors(const expr_vector& v)
  {
    return successors(extract_state(v));
  }

  set<State> Model::successors(const State& s)
  {
    using mysat::primed::lit_type;
    using std::optional;
    using z3ext::solver::check_witness;

    set<State> S;

    z3::solver solver(ctx);
    solver.add(s.cube(*this));

    while (optional<expr_vector> w = check_witness(solver, transition))
    {
      S.insert(extract_state(*w, lit_type::primed));
      solver.add(!mk_and(*w)); // exclude from future search
    }

    return S;
  }

  void Model::test_room()
  {
    using mysat::primed::lit_type;
    using std::cout;
    using std::endl;
    using std::map;
    using std::queue;
    using std::set;

    std::ofstream out("peter-out.txt");
    // std::ostream& out = cout;
    cout << "N = " << N << std::endl;

    queue<State> Q;
    set<State> visited;
    map<State, set<State>> edges;
    // create a set if none exists, and insert destination
    auto edges_insert = [&edges](const State& src, const State& dst)
    {
      set<State> s;
      set<State>& dst_set = edges.emplace(src, s).first->second;
      dst_set.insert(dst);
    };

    const State I = extract_state(initial);
    Q.push(I);

    while (not Q.empty())
    {
      const State& source = Q.front();
      if (visited.insert(source).second) // if source was not done already
      {
        for (const State& dest : successors(source))
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
      assert(map_pair.second.size() <= N);

      string src_str = map_pair.first.inline_string();

      for (const State& dst : map_pair.second)
        out << fmt::format("\"{}\" -> \"{}\"", src_str, dst.inline_string())
            << endl
            << endl;
    }
    out << "}" << endl;
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
      expr_vector increment(ctx);
      for (numrep_t x = 0; x < N - 1; x++)
      {
        expr set_index =
            implies(level.at(i).equals(x), level.at(i).p_equals(x + 1));
        // expr rest_stays =
        //     implies(!level.at(i).equals(x), level.at(i).unchanged()); // uhmm
        increment.push_back(set_index);
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

  void Model::bv_comp_test(size_t max_value)
  {
    mysat::primed::BitVec bv1(ctx, "b1", bits_for(max_value + 1));
    mysat::primed::BitVec bv2(ctx, "b2", bits_for(max_value + 1));
    unsigned wrong{ 0 };

    for (unsigned i = 0; i <= max_value; i++)
    {
      for (unsigned j = 0; j <= max_value; j++)
      {
        z3::solver s(ctx);

        s.add(bv1.equals(i));
        s.add(bv2.equals(j));
        s.add(bv1.less(bv2));
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
    std::cout << fmt::format("{} wrong bv comparisons", wrong) << std::endl;
    return;
  }

  void Model::bv_val_test(size_t max_value)
  {
    mysat::primed::BitVec bv(ctx, "b", bits_for(max_value + 1));
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
} // namespace peterson
