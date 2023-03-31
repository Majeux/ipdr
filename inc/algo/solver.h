#ifndef SOLVER_H
#define SOLVER_H
#include "pdr-context.h"
#include "z3-ext.h"

#include <exception>
#include <fmt/core.h>
#include <memory>
#include <numeric>
#include <set>
#include <vector>
#include <z3++.h>

namespace pdr
{
  enum class SolverState
  {
    fresh,             // neither is available
    witness_available, // only witness, not core
    core_available,    // only core, not witness
  };

  class Solver
  {
   public:
    Solver(Context& ctx, const IModel& m, z3::expr_vector base,
        z3::expr_vector t, z3::expr_vector con);

    void remake(z3::expr_vector base, z3::expr_vector transition,
        z3::expr_vector constraint);
    void reset();
    void reset(const z3ext::CubeSet& cubes);
    // sets a new ccnf constraint, removes all blocked cubes
    void reconstrain_clear(z3::expr_vector constraint);
    // adds a cube's clause to the solver
    void block(const z3::expr_vector& cube);
    void block(const z3::expr_vector& cube, const z3::expr& act);
    void block(const std::vector<z3::expr>& cube);
    void block(const std::vector<z3::expr>& cube, const z3::expr& act);
    void block(const z3ext::CubeSet& cubes, const z3::expr& act);

    bool SAT(const z3::expr_vector& assumptions);
    z3::model get_model() const;
    z3::model witness_raw() const;
    z3::expr_vector witness_current() const;
    std::vector<z3::expr> std_witness_current() const;
    std::vector<z3::expr> witness_current_intersect(
        const std::vector<z3::expr>& vec) const;

    std::string as_str(const std::string& header, bool clauses_only) const;

    // function to extract a cube representing a satisfying assignment to
    // the last SAT call to the solver. the resulting vector or expr_vector
    // is in sorted order template UnaryPredicate: function expr->bool to
    // filter atoms. accepts 1 expr, returns bool
    template <typename UnaryPredicate>
    static z3::expr_vector filter_witness(const z3::model& m, UnaryPredicate p);
    template <typename UnaryPredicate>
    static std::vector<z3::expr> filter_witness_vector(
        const z3::model& m, UnaryPredicate p);

    // function extract the unsat_core from the solver, a subset of the
    // assumptions the resulting vector or expr_vector is in sorted order
    // assumes a core is only extracted once
    // ! result is sorted
    std::vector<z3::expr> unsat_core() const;
    // template UnaryPredicate: function expr->bool to filter literals from
    // the core template Transform: function expr->expr. each literal is
    // replaced by result before pushing
    template <typename UnaryPredicate, typename Transform>
    z3::expr_vector unsat_core(UnaryPredicate p, Transform t);

   private:
    class InvalidExtraction : public std::exception
    {
     private:
      std::string message{ "Solver::InvalidExtraction" };

      void explain(std::string_view msg)
      {
        message += fmt::format(": {}", msg);
      }

     public:
      InvalidExtraction(std::string_view msg) : message() { explain(msg); }
      InvalidExtraction(SolverState s)
      {
        switch (s)
        {
          case SolverState::fresh:
            explain("witness and core unavailable");
            break;
          case SolverState::witness_available:
            explain("core unavailable");
            break;
          case SolverState::core_available:
            explain("witness unavailable");
            break;
        }
      }

      static InvalidExtraction NonConstant(const z3::expr& e)
      {
        return InvalidExtraction(
            fmt::format("witness contains non-constant: {}", e.to_string()));
      }

      const char* what() const noexcept override { return message.c_str(); }
    };

    const mysat::primed::VarVec& vars;
    z3::solver internal_solver;
    SolverState state{ SolverState::fresh };
    unsigned clauses_start; // point where base_assertions ends and other
                            // assertions begin

  };

  template <typename UnaryPredicate>
  z3::expr_vector Solver::filter_witness(const z3::model& m, UnaryPredicate p)
  {
    std::vector<z3::expr> std_vec = filter_witness_vector(m, p);
    z3::expr_vector v(std_vec[0].ctx());
    for (const z3::expr& e : std_vec)
      v.push_back(e);
    return v;
  }

  template <typename UnaryPredicate>
  std::vector<z3::expr> Solver::filter_witness_vector(
      const z3::model& m, UnaryPredicate p)
  {
    std::vector<z3::expr> v;
    v.reserve(m.num_consts());
    for (unsigned i = 0; i < m.size(); i++)
    {
      z3::func_decl f  = m[i];
      z3::expr b_value = m.get_const_interp(f);
      z3::expr literal = f();
      if (p(literal))
      {
        if (b_value.is_true())
          v.push_back(literal);
        else if (b_value.is_false())
          v.push_back(!literal);
        else
          throw InvalidExtraction::NonConstant(b_value);
      }
    }
    z3ext::order_lits(v);

    return v;
  }

  // template <typename UnaryPredicate, typename Transform>
  // z3::expr_vector Solver::unsat_core(UnaryPredicate filter, Transform transform)
  // {
  //   z3::expr_vector full_core = unsat_core();

  //   if (full_core.size() == 0)
  //     return full_core;

  //   std::vector<z3::expr> core;
  //   core.reserve(full_core.size());
  //   for (const z3::expr& e : full_core)
  //   {
  //     if (filter(e))
  //       core.push_back(transform(e));
  //   }

  //   z3ext::order_lits(core);

  //   return z3ext::convert(core);
  // }
} // namespace pdr
#endif // SOLVER_H
