#ifndef Z3_EXT
#define Z3_EXT

#include <algorithm>
#include <fmt/core.h>
#include <sstream>
#include <vector>
#include <z3++.h>
#include <set>

namespace z3ext
{
  z3::expr minus(const z3::expr& e);

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

  void sort(z3::expr_vector& v);

  // returns true if l < r
  // assumes l and r are in sorted order (as sort())
  bool subsumes_l(const z3::expr_vector& l, const z3::expr_vector& r);
  // returns true if l <= r
  // assumes l and r are in sorted order (as sort())
  bool subsumes_le(const z3::expr_vector& l, const z3::expr_vector& r);

  // COMPARATOR FUNCTORS
  //
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
  template <typename ExprVector> inline std::vector<std::string> to_strings(ExprVector v)
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
} // namespace z3ext
#endif // Z3_EXT
