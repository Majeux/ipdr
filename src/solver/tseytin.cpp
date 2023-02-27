// Adapted from
// https://github.com/lsils/bill/blob/master/include/bill/sat/tseytin.hpp
/*
MIT License

Copyright (c) 2019, LSI Group at EPFL

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*! \brief Adds CNF clauses for `y = (l_0 and ... and l_{n-1})` to the solver.
 *
 * \param solver Solver
 * \param ls List of literals
 * \return Literal y
 */
#include "solver.h"
namespace pdr
{
  // /*! \brief Adds CNF clauses for `y = (a and b)` to the solver.
  //  *
  //  * \param solver Solver
  //  * \param a Literal
  //  * \param b Literal
  //  * \return Literal y
  //  */
  // template <typename Solver>
  // lit_type add_tseytin_and(Solver& solver, lit_type const& a, lit_type const& b)
  // {
  //   auto const r = solver.add_variable();
  //   solver.add_clause(
  //       std::vector{ ~a, ~b, lit_type(r, lit_type::polarities::positive) });
  //   solver.add_clause(
  //       std::vector{ a, lit_type(r, lit_type::polarities::negative) });
  //   solver.add_clause(
  //       std::vector{ b, lit_type(r, lit_type::polarities::negative) });
  //   return lit_type(r, lit_type::polarities::positive);
  // }

  // /*! \brief Adds CNF clauses for `y = (l_0 and ... and l_{n-1})` to the solver.
  //  *
  //  * \param solver Solver
  //  * \param ls List of literals
  //  * \return Literal y
  //  */
  // template <typename Solver>
  // lit_type add_tseytin_and(Solver& solver, std::vector<lit_type> const& ls)
  // {
  //   auto const r = solver.add_variable();
  //   std::vector<lit_type> cls;
  //   for (const auto& l : ls)
  //     cls.emplace_back(~l);
  //   cls.emplace_back(lit_type(r, lit_type::polarities::positive));
  //   solver.add_clause(cls);
  //   for (const auto& l : ls)
  //     solver.add_clause(
  //         std::vector{ l, lit_type(r, lit_type::polarities::negative) });
  //   return lit_type(r, lit_type::polarities::positive);
  // }

  // /*! \brief Adds CNF clauses for `y = a or b` to the solver.
  //  *
  //  * \param solver Solver
  //  * \param a Literal
  //  * \param b Literal
  //  * \return Literal y
  //  */
  // template <typename Solver>
  // lit_type add_tseytin_or(Solver& solver, lit_type const& a, lit_type const& b)
  // {
  //   auto const r = solver.add_variable();
  //   solver.add_clause(
  //       std::vector{ a, b, lit_type(r, lit_type::polarities::negative) });
  //   solver.add_clause(
  //       std::vector{ ~a, lit_type(r, lit_type::polarities::positive) });
  //   solver.add_clause(
  //       std::vector{ ~b, lit_type(r, lit_type::polarities::positive) });
  //   return lit_type(r, lit_type::polarities::positive);
  // }

  // /*! \brief Adds CNF clauses for `y = (l_0 or ... or l_{n-1})` to the solver.
  //  *
  //  * \param solver Solver
  //  * \param ls List of literals
  //  * \return Literal y
  //  */
  // template <typename Solver>
  // lit_type add_tseytin_or(Solver& solver, std::vector<lit_type> const& ls)
  // {
  //   auto const r = solver.add_variable();
  //   std::vector<lit_type> cls(ls);
  //   cls.emplace_back(lit_type(r, lit_type::polarities::negative));
  //   solver.add_clause(cls);
  //   for (const auto& l : ls)
  //     solver.add_clause(
  //         std::vector{ ~l, lit_type(r, lit_type::polarities::positive) });
  //   return lit_type(r, lit_type::polarities::positive);
  // }

  // /*! \brief Adds CNF clauses for `y = (a xor b)` to the solver.
  //  *
  //  * \param solver Solver
  //  * \param a Literal
  //  * \param b Literal
  //  * \return Literal y
  //  */
  // template <typename Solver>
  // lit_type add_tseytin_xor(Solver& solver, lit_type const& a, lit_type const& b)
  // {
  //   auto const r = solver.add_variable();
  //   solver.add_clause(
  //       std::vector{ ~a, ~b, lit_type(r, lit_type::polarities::negative) });
  //   solver.add_clause(
  //       std::vector{ ~a, b, lit_type(r, lit_type::polarities::positive) });
  //   solver.add_clause(
  //       std::vector{ a, ~b, lit_type(r, lit_type::polarities::positive) });
  //   solver.add_clause(
  //       std::vector{ a, b, lit_type(r, lit_type::polarities::negative) });
  //   return lit_type(r, lit_type::polarities::positive);
  // }

  // /*! \brief Adds CNF clauses for `y = (a == b)` to the solver.
  //  *
  //  * \param solver Solver
  //  * \param a Literal
  //  * \param b Literal
  //  * \return Literal y
  //  */
  // template <typename Solver>
  // lit_type add_tseytin_equals(Solver& solver, lit_type const& a,
  //                             lit_type const& b)
  // {
  //   auto const r = solver.add_variable();
  //   solver.add_clause(
  //       std::vector{ ~a, ~b, lit_type(r, lit_type::polarities::positive) });
  //   solver.add_clause(
  //       std::vector{ ~a, b, lit_type(r, lit_type::polarities::negative) });
  //   solver.add_clause(
  //       std::vector{ a, ~b, lit_type(r, lit_type::polarities::negative) });
  //   solver.add_clause(
  //       std::vector{ a, b, lit_type(r, lit_type::polarities::positive) });
  //   return lit_type(r, lit_type::polarities::positive);
  // }

} // namespace pdr
