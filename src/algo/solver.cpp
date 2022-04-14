#include "solver.h"
#include "frame.h"
#include <z3++.h>

namespace pdr
{
#warning still duplicates in solver in dump
  Solver::Solver(const Context& c, z3::expr_vector base,
                 z3::expr_vector transition, z3::expr_vector constraint)
      : ctx(c), internal_solver(ctx()), state(SolverState::neutral)
  {
    internal_solver.set("sat.cardinality.solver", true);
    //  TODO sat.core.minimize
    internal_solver.set("cardinality.solver", true);
    internal_solver.set("sat.random_seed", ctx.seed);
    // consecution_solver.set("lookahead_simplify", true);

    // backtracking point to solver without constraints or blocked states
    internal_solver.add(base);
    internal_solver.add(transition);
    internal_solver.push();
    // backtracking point to solver without blocked states
    internal_solver.add(constraint);
    internal_solver.push();

    clauses_start = base.size() + transition.size() + constraint.size();
  }

  void Solver::reset()
  {
    internal_solver.pop();  // remove all blocked states
    internal_solver.push(); // remake backtracking point
    // internal_solver.reset();
    // init();
  }

  // reset and automatically repopulate by blocking cubes
  // used by frames
  void Solver::reset(const CubeSet& cubes)
  {
    reset();
    for (const z3::expr_vector& cube : cubes)
      block(cube);
  }

  void Solver::reconstrain(z3::expr_vector constraint)
  {
    internal_solver.pop(2);          // remove all blocked cubes and constraint
    internal_solver.push();          // remake constraintless backtracking point
    internal_solver.add(constraint);
    internal_solver.push();          // remake stateless backtracking point
    clauses_start = internal_solver.assertions().size();
  }

  void Solver::reconstrain(z3::expr_vector constraint, const CubeSet& cubes)
  {
    reconstrain(constraint);
    for (const z3::expr_vector& cube : cubes)
      block(cube);
  }

  void Solver::block(const z3::expr_vector& cube)
  {
    z3::expr clause = z3::mk_or(z3ext::negate(cube));
    this->add(clause);
  }

  void Solver::block(const z3::expr_vector& cube, const z3::expr& act)
  {
    z3::expr clause = z3::mk_or(z3ext::negate(cube));
    this->add(clause | !act);
  }

  void Solver::add(const z3::expr& e) { internal_solver.add(e); }

  bool Solver::SAT(const z3::expr_vector& assumptions)
  {
    z3::check_result result = internal_solver.check(assumptions);
    if (result == z3::sat)
      return true;

    // if (result == z3::check_result::unknown)
    // {
      // std::cout << as_str("", true);
      assert(result != z3::check_result::unknown);
    // }

    core_available = true;
    return false;
  }

  z3::model Solver::get_model() const { return internal_solver.get_model(); }

  // TODO optional return
  z3::expr_vector Solver::unsat_core()
  {
    assert(core_available);
    z3::expr_vector core = internal_solver.unsat_core();
    core_available       = false;
    return core;
  }

  z3::expr_vector Solver::witness_current() const
  {
    z3::model m = internal_solver.get_model();
    std::vector<z3::expr> std_vec;
    std_vec.reserve(m.num_consts());

    for (unsigned i = 0; i < m.size(); i++)
    {
      z3::func_decl f        = m[i];
      z3::expr boolean_value = m.get_const_interp(f);
      z3::expr literal       = f();

      if (ctx.model().lits.atom_is_current(literal))
      {
        if (boolean_value.is_true())
          std_vec.push_back(literal);
        else if (boolean_value.is_false())
          std_vec.push_back(!literal);
        else
          throw std::runtime_error("model contains non-constant");
      }
    }
    std::sort(std_vec.begin(), std_vec.end(), z3ext::expr_less());
    z3::expr_vector v(ctx());
    for (const z3::expr& e : std_vec)
      v.push_back(e);
    return v;
  }

  std::string Solver::as_str(const std::string& header, bool clauses_only) const
  {
    std::stringstream ss;
    ss << header << std::endl;
    ss << "z3::statistics" << std::endl;
    ss << internal_solver.statistics() << std::endl;

    const z3::expr_vector asserts = internal_solver.assertions();
    auto it = asserts.begin();
    if (clauses_only) // skip base, transition and constraint
    {
      for (unsigned i = 0; i < clauses_start && it != asserts.end(); i++)
        it++;
    }

    for (; it != asserts.end(); it++)
      ss << fmt::format("- {}", (*it).to_string()) << std::endl;

    return ss.str();
  }
} // namespace pdr
