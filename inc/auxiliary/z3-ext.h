#ifndef Z3_EXT
#define Z3_EXT

#include <algorithm>
#include <fmt/core.h>
#include <optional>
#include <set>
#include <sstream>
#include <vector>
#include <z3++.h>

namespace z3ext
{
  z3::expr minus(const z3::expr& e);
  bool is_lit(const z3::expr& e);

  // allocates new vector
  // by default, assignment and copy constructors copy a reference to an
  // internal vector. This constructs a deep copy, preventing the original
  // from being altered
  z3::expr_vector copy(const z3::expr_vector& v);

  // allocates new vector
  // negate every literal in the vector
  z3::expr_vector negate(const z3::expr_vector& lits);
  z3::expr_vector negate(const std::vector<z3::expr>& lits);

  // allocates new vector
  // convert between the z3 internal and std::vector representation
  z3::expr_vector convert(const std::vector<z3::expr>& vec);
  z3::expr_vector convert(std::vector<z3::expr>&& vec);
  std::vector<z3::expr> convert(const z3::expr_vector& vec);

  // allocates new vector
  // list all arguments of an expression
  z3::expr_vector args(const z3::expr& e);

  // sort based on literals
  void sort(z3::expr_vector& v);

  // sort based on symbols
  // @ v is a cube of literals
  void sort_cube(z3::expr_vector& v);

  // returns true if l < r
  // assumes l and r are in sorted order (as sort())
  bool subsumes_l(const z3::expr_vector& l, const z3::expr_vector& r);
  // returns true if l <= r
  // assumes l and r are in sorted order (as sort())
  bool subsumes_le(const z3::expr_vector& l, const z3::expr_vector& r);

  // COMPARATOR FUNCTORS
  //
  // compares literals by their symbol
  struct lit_less
  {
    bool operator()(const z3::expr& l, const z3::expr& r) const;
  };

  // z3::expr comparator
  struct expr_less
  {
    bool operator()(const z3::expr& l, const z3::expr& r) const;
  };

  // z3::expr_vector comparator
  struct expr_vector_less
  {
    bool operator()(const z3::expr_vector& l, const z3::expr_vector& r) const;
  };

  // TEMPLATE FUNCTIONS
  //
  template <typename ExprVector>
  inline std::vector<std::string> to_strings(ExprVector v)
  {
    std::vector<std::string> strings;
    strings.reserve(v.size());
    std::transform(v.begin(), v.end(), std::back_inserter(strings),
        [](const z3::expr& i) { return i.to_string(); });

    return strings;
  }

  // return a string representation of a vector of z3::expr
  template <typename ExprVector>
  inline std::string join_expr_vec(const ExprVector& c, bool align = true,
      const std::string delimiter = ", ")
  {
    // only join containers that can stream into stringstream
    if (c.size() == 0)
      return "";

    std::vector<std::string> strings = to_strings(c);

    bool first = true;
    unsigned largest;
    for (const std::string& s : strings)
    {
      if (first || s.length() > largest)
        largest = s.length();
      first = false;
    }

    first = true;
    std::stringstream ss;
    for (const std::string& s : strings)
    {
      if (!first)
        ss << delimiter;
      first = false;
      ss << fmt::format("{: ^{}}", s, align ? largest : 0);
    }
    return ss.str();
  }

  using CubeSet = std::set<z3::expr_vector, expr_vector_less>;

  namespace solver
  {
    // retrieve the current model in the solver as a cube
    // the resulting cube is sorted by id()
    // @ s has just completed a satisfiable check()
    z3::expr_vector get_witness(const z3::solver& s);
    std::vector<z3::expr> get_std_witness(const z3::solver& s);

    // perform a solver.check() and return the resulting witness
    std::optional<z3::expr_vector> check_witness(z3::solver& s);
    std::optional<z3::expr_vector> check_witness(
        z3::solver& s, const z3::expr_vector& assumptions);

    template <typename UnaryPredicate>
    std::vector<z3::expr> std_witness_st(const z3::model& m, UnaryPredicate p)
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
            throw std::runtime_error("model contains non-constant");
        }
      }
      std::sort(v.begin(), v.end(), z3ext::expr_less());
      return v;
    }

    template <typename UnaryPredicate>
    z3::expr_vector witness_st(const z3::solver& s, UnaryPredicate p)
    {
      return convert(std_witness_st(s, p));
    }
  } // namespace solver

  namespace tseytin
  {
    // convert e to cnf using z3's simplify and tseitin conversion tactics
    z3::expr_vector to_cnf_vec(const z3::expr& e);
    z3::expr to_cnf(const z3::expr& e);

    // add a tseytin encoded AND statement to "cnf"
    // c = a & b <=> (!a | !b | c) & (a | !c) & (b | !c)
    z3::expr add_and(z3::expr_vector& cnf, const std::string& name,
        const z3::expr& a, const z3::expr& b);

    // add a tseytin encoded OR statement to "cnf"
    // c = a | b <=> (a | b | !c) & (!a | c) & (!b | c)
    z3::expr add_or(z3::expr_vector& cnf, const std::string& name,
        const z3::expr& a, const z3::expr& b);

    // add a tseytin encoded IMPLIES statement to "cnf"
    // a => b <=> !a | b
    z3::expr add_implies(z3::expr_vector& cnf, const std::string& name,
        const z3::expr& a, const z3::expr& b);

    // add a tseytin encoded XOR statement to "cnf"
    // c = a ^ b <=> (!a | !b | !c) & (a | b | !c) & (a | !b | c) & (!a | b | c)
    z3::expr add_xor(z3::expr_vector& cnf, const std::string& name,
        const z3::expr& a, const z3::expr& b);

    // add a tseytin encoded XNOR statement to "cnf"
    // c = a ^ b <=> (!a | !b | c) & (a | b | c) & (a | !b | !c) & (!a | b | !c)
    z3::expr add_xnor(z3::expr_vector& cnf, const std::string& name,
        const z3::expr& a, const z3::expr& b);
  } // namespace tseytin
} // namespace z3ext
#endif // Z3_EXT
