#include "solver.h"
#include "frame.h"
#include <z3++.h>

namespace pdr
{
  Solver::Solver(context& c, std::vector<z3::expr_vector> base)
      : ctx(c), internal_solver(ctx()), base_assertions(std::move(base))
  {
    init();
  }

  void Solver::init()
  {
    internal_solver.set("sat.cardinality.solver", true);
    //  TODO sat.core.minimize
    internal_solver.set("cardinality.solver", true);
    internal_solver.set("sat.random_seed", ctx.seed);
    // consecution_solver.set("lookahead_simplify", true);
    for (const z3::expr_vector& v : base_assertions)
      internal_solver.add(v);

    cubes_start = std::accumulate(
        base_assertions.begin(), base_assertions.end(), 0,
        [](int agg, const z3::expr_vector& v) { return agg + v.size(); });
  }

  void Solver::reset()
  {
    internal_solver.reset();
    init();
  }

  // reset and automatically repopulate by blocking cubes
  // used by frames
  void Solver::reset(const CubeSet& cubes)
  {
    reset();
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

    assert(result != z3::check_result::unknown);

    core_available = true;
    return false;
  }

  z3::model Solver::get_model() const { return internal_solver.get_model(); }

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
      z3::func_decl f = m[i];
      z3::expr boolean_value = m.get_const_interp(f);
      z3::expr literal = f();

      if (ctx.const_model().literals.atom_is_current(literal)) 
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

  std::string Solver::as_str(const std::string& header) const
  {
    std::string str(header);
    const z3::expr_vector asserts = internal_solver.assertions();

    auto it = asserts.begin();
    for (unsigned i = 0; i < cubes_start && it != asserts.end(); i++)
      it++;

    for (; it != asserts.end(); it++)
      str += fmt::format("- {}\n", (*it).to_string());

    return str;
  }
} // namespace pdr
