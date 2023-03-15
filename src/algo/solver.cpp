#include "solver.h"
#include "frame.h"
#include "z3-ext.h"
#include <algorithm>
#include <z3++.h>

#include <spdlog/spdlog.h>
#include <vector>

namespace pdr
{
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

#warning still duplicates in solver in dump
  Solver::Solver(Context& ctx, const IModel& m, expr_vector base,
      expr_vector transition, expr_vector constraint)
      : vars(m.vars), internal_solver(ctx)
  {
    internal_solver.set("sat.random_seed", ctx.seed);
    internal_solver.set("sat.cardinality.solver", true);
    // consecution_solver.set("lookahead_simplify", true);
    remake(base, transition, constraint);
  }

  void Solver::remake(
      expr_vector base, expr_vector transition, expr_vector constraint)
  {
    internal_solver.reset();
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
  void Solver::reset(const z3ext::CubeSet& cubes)
  {
    reset();
    for (const expr_vector& cube : cubes)
      block(cube);
  }

  void Solver::reconstrain_clear(expr_vector constraint)
  {
    internal_solver.pop(2); // remove all blocked cubes and constraint
    internal_solver.push(); // remake constraintless backtracking point
    internal_solver.add(constraint);
    internal_solver.push(); // remake stateless backtracking point
    clauses_start = internal_solver.assertions().size();
  }

  void Solver::block(const expr_vector& cube)
  {
    expr clause = z3::mk_or(z3ext::negate(cube));
    internal_solver.add(clause);
  }

  void Solver::block(const expr_vector& cube, const expr& act)
  {
    expr clause = z3::mk_or(z3ext::negate(cube));
    internal_solver.add(clause | !act);
  }

  void Solver::block(const z3ext::CubeSet& cubes, const expr& act)
  {
    for (const expr_vector& cube : cubes)
      block(cube, act);
  }

  bool Solver::SAT(const expr_vector& assumptions)
  {
    state                   = SolverState::fresh;
    z3::check_result result = internal_solver.check(assumptions);
    if (result == z3::sat)
    {
      state = SolverState::witness_available;
      return true;
    }

    assert(result != z3::check_result::unknown);

    state = SolverState::core_available;
    return false;
  }

  z3::model Solver::get_model() const { return internal_solver.get_model(); }

  // TODO optional return
  expr_vector Solver::unsat_core() const
  {
    // if(state != SolverState::core_available)
    //   throw InvalidExtraction(state);

    expr_vector core = internal_solver.unsat_core();
    z3ext::order_lits(core);

    return core;
  }

  vector<expr> Solver::std_witness_current() const
  {
    if(state != SolverState::witness_available)
      throw InvalidExtraction(state);

    z3::model m = internal_solver.get_model();
    vector<expr> std_vec;
    std_vec.reserve(m.num_consts());

    for (unsigned i = 0; i < m.size(); i++)
    {
      z3::func_decl f    = m[i];
      expr boolean_value = m.get_const_interp(f);
      expr literal       = f();

      if (vars.lit_is_current(literal))
      {
        if (boolean_value.is_true())
          std_vec.push_back(literal);
        else if (boolean_value.is_false())
          std_vec.push_back(!literal);
        else
          throw InvalidExtraction::NonConstant(boolean_value);
      }
    }

    z3ext::order_lits(std_vec);
    return std_vec;
  }

  expr_vector Solver::witness_current() const
  {
    return z3ext::convert(std_witness_current());
  }

  vector<expr> Solver::witness_current_intersect(const vector<expr>& ev) const
  {
    using z3ext::join_ev;
    using z3ext::lit_less;

    if(state != SolverState::witness_available)
      throw InvalidExtraction(state);

    assert(z3ext::lits_ordered(ev));

    z3::model m = internal_solver.get_model();

    vector<expr> std_vec;
    std_vec.reserve(m.num_consts());

    for (unsigned i = 0; i < m.size(); i++)
    {
      if (std_vec.size() >= ev.size()) // intersection cannot be larger than ev
        break;

      z3::func_decl f    = m[i];
      expr boolean_value = m.get_const_interp(f);
      expr var           = f();

      if (vars.lit_is_current(var))
      {
        expr literal(var.ctx());
        if (boolean_value.is_true())
          literal = var;
        else if (boolean_value.is_false())
          literal = !var;
        else
          throw InvalidExtraction::NonConstant(boolean_value);
      
        // search for 
        if (std::binary_search(ev.begin(), ev.end(), literal, z3ext::cube_orderer))
          std_vec.push_back(literal);
      }
    }

    z3ext::order_lits(std_vec);
    return std_vec;
  }

  std::string Solver::as_str(const std::string& header, bool clauses_only) const
  {
    std::stringstream ss;
    ss << header << std::endl;
    ss << "z3::statistics" << std::endl;
    ss << internal_solver.statistics() << std::endl;

    const expr_vector asserts = internal_solver.assertions();
    auto it                   = asserts.begin();
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
