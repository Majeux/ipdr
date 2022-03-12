#ifndef Z3_EXT
#define Z3_EXT

#include <algorithm>
#include <sstream>
#include <vector>
#include <z3++.h>

namespace z3ext
{
  using std::string;
  using std::vector;
  using z3::expr;
  using z3::expr_vector;

  expr minus(const expr& e);

  // allocates new vector
  // by default, assignment and copy constructors copy a reference to an
  // internal vector. This constructs a deep copy, preventing the original
  // from being altered
  expr_vector copy(const expr_vector& v);

  // allocates new vector
  // negate every literal in the vector
  expr_vector negate(const expr_vector& lits);
  expr_vector negate(const vector<expr>& lits);

  // allocates new vector
  // convert between the z3 internal and std::vector representation
  expr_vector convert(const vector<expr>& vec);
  expr_vector convert(vector<expr>&& vec);
  vector<expr> convert(const expr_vector& vec);

  // allocates new vector
  // list all arguments of an expression
  expr_vector args(const expr& e);

  void sort(expr_vector& v);

  // returns true if l <= r
  // assumes l and r are in sorted order (as sort())
  bool subsumes(const expr_vector& l, const expr_vector& r);

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
    bool operator()(const expr_vector& l, const expr_vector& r) const;
  };

  // TEMPLATE FUNCTIONS
  //
  template <typename ExprVector> inline vector<string> to_strings(ExprVector v)
  {
    vector<string> strings;
    strings.reserve(v.size());
    std::transform(v.begin(), v.end(), std::back_inserter(strings),
                   [](const z3::expr& i) { return i.to_string(); });

    return strings;
  }

  // return a string representation of a vector of z3::expr
  template <typename ExprVector>
  inline string join_expr_vec(const ExprVector& c, bool align = true,
                              const string delimiter = ", ")
  {
    // only join containers that can stream into stringstream
    if (c.size() == 0)
      return "";

    vector<string> strings = to_strings(c);

    bool first = true;
    unsigned largest;
    for (const string& s : strings)
    {
      if (first || s.length() > largest)
        largest = s.length();
      first = false;
    }

    first = true;
    std::stringstream ss;
    for (const string& s : strings)
    {
      if (!first)
        ss << delimiter;
      first = false;
      if (align)
        ss << string(largest - s.length(), ' ');
      ss << s;
    }
    return ss.str();
  }
} // namespace z3ext
#endif // Z3_EXT
