#ifndef Z3_EXT
#define Z3_EXT

#include "string-ext.h"
#include <algorithm>
#include <cstring>
#include <fmt/core.h>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <type_traits>
#include <vector>
#include <z3++.h>

namespace z3ext
{
  // atoms and literals
  //
  struct LitStr
  {
    std::string atom;
    bool sign;
    // extract the string representation and sign from a literal expressions
    LitStr(std::string_view a, bool s);
    LitStr(z3::expr const& l);
    // LitStr(LitStr const&) = default;
    z3::expr to_expr(z3::context& ctx);
    std::string to_string() const;
  };

  // handling cubes of the form: < c1, c2, ..., c3, constraint_lit >
  namespace constrained_cube
  {
    namespace
    {
      int constexpr length(const char* str)
      {
        return *str ? 1 + length(str + 1) : 0;
      }
    } // namespace

    // compile time definitions
    static constexpr const char* tag_chars   = "__c__";
    constexpr static const size_t tag_length = length(tag_chars);
    // used definitions
    inline static const std::string tag(tag_chars); // usage: __c{}__
    // all except last two characters are prefix
    inline static const std::string prefix = tag.substr(0, tag.length() - 2);
    // last two are suffix
    inline static const std::string suffix = tag.substr(tag.length() - 2, 2);

    std::string constraint_str(size_t size);
    std::optional<size_t> constraint_size(std::string_view str);
    std::optional<size_t> constraint_size(z3::expr const& e);
    // true if a is stronger than be
    // @pre: a and b in form < c1, c2, ..., cn, constraint >
    bool stronger_constraint(z3::expr const& a, z3::expr const& b);

    // copy construction.
    // @pre: lits in form < c1, c2, ..., cn, constraint >.
    // @post: rv in form < c1, c2, ..., cn, constraint >.
    std::vector<z3::expr> mk_constrained_cube(
        std::vector<z3::expr> const& lits, size_t size);
    // move construction.
    // @pre: lits in form < c1, c2, ..., cn, constraint >.
    // @post: rv in form < c1, c2, ..., cn, constraint >.
    std::vector<z3::expr> mk_constrained_cube(
        std::vector<z3::expr>&& lits, size_t size);

    // true if a is stronger than b.
    // @pre: a and b in form < c1, c2, ..., cn, constraint >
    bool subsumes_le(
        std::vector<z3::expr> const& a, std::vector<z3::expr> const& b);

    // compares constrained cubes
    // cubes in form < c1, c2, ..., cn, constraint >
    struct cexpr_less
    {
      bool operator()(z3::expr const& a, z3::expr const& b) const;
    };

    // vectors can be sorted as normal by id

  }; // namespace constrained_cube

  z3::expr minus(z3::expr const& e);
  bool is_lit(z3::expr const& e);
  // get the variable of a literal.
  // @pre: e is a literal
  z3::expr strip_not(z3::expr const& e);

  // allocates new vector
  // negate every literal in the vector
  z3::expr_vector negate(const z3::expr_vector& lits);
  z3::expr_vector negate(const std::vector<z3::expr>& lits);

  // allocates new vector
  // convert between the z3 internal and std::vector representation
  z3::expr_vector convert(const std::vector<z3::expr>& vec);
  z3::expr_vector convert(std::vector<z3::expr>&& vec);
  std::vector<z3::expr> convert(const z3::expr_vector& vec);

  // ! allocates new vector
  // list all arguments of an expression
  z3::expr_vector args(const z3::expr& e);

  // sort based on var of a literal
  // @ cube is a vector of literals
  void sort_lits(std::vector<z3::expr>& cube);
  void sort_lits(z3::expr_vector& cube);

  // sort based on full expressions
  void sort_exprs(std::vector<z3::expr>& v);
  void sort_exprs(z3::expr_vector& v);

  // returns true if l < r
  // assumes l and r are in sorted order (as sort())
  bool subsumes_l(const z3::expr_vector& l, const z3::expr_vector& r);
  bool subsumes_l(
      const std::vector<z3::expr>& l, const std::vector<z3::expr>& r);
  // returns true if l <= r
  // assumes l and r are in sorted order (as sort())
  bool subsumes_le(const z3::expr_vector& l, const z3::expr_vector& r);
  bool subsumes_le(
      const std::vector<z3::expr>& l, const std::vector<z3::expr>& r);

  bool eq(const z3::expr_vector& l, const z3::expr_vector& r);
  bool quick_implies(const z3::expr_vector& l, const z3::expr_vector& r);

  // COMPARATOR FUNCTORS
  //
  // compares literals by their atom
  struct lit_less
  {
    bool operator()(const z3::expr& l, const z3::expr& r) const;
  };

  // z3::expr comparator
  struct expr_less
  {
    bool operator()(const z3::expr& l, const z3::expr& r) const;
  };

  // z3::expr hash function
  struct expr_hash
  {
    size_t operator()(const z3::expr& l) const;
  };

  // z3::expr_vector comparator
  struct expr_vector_less
  {
    bool operator()(const z3::expr_vector& l, const z3::expr_vector& r) const;
  };

  // vector<expr> comparator
  struct std_expr_vector_less
  {
    bool operator()(
        const std::vector<z3::expr>& l, const std::vector<z3::expr>& r) const;
  };

  // internal cube order by pdr
  //
  // the default less-than comparison used to order cubes
  inline expr_less cube_orderer;
  // for extra-constrained cubes (relaxing)
  // inline constrained_cube::cexpr_less cube_orderer;
  void order_lits(std::vector<z3::expr>& cube);
  void order_lits(z3::expr_vector& cube);
  std::vector<z3::expr> order_lits_std(z3::expr_vector const& cube);
  bool lits_ordered(std::vector<z3::expr> const& cube);

  // TEMPLATE FUNCTIONS
  //
  template <typename ExprVector>
  inline std::vector<std::string> to_strings(ExprVector v)
  {
    std::vector<std::string> strings;
    strings.reserve(v.size());
    std::transform(v.begin(), v.end(), std::back_inserter(strings),
        [](z3::expr const& i) { return i.to_string(); });

    return strings;
  }

  template <typename ExprVector>
  inline std::string join_ev_aligned(
      ExprVector const& c, const std::string delimiter = ", ")
  {

    std::vector<std::string> strings = to_strings(c);

    unsigned largest =
        std::max_element(strings.cbegin(), strings.cend(), str::ext::size_lt)
            ->size();

    std::stringstream ss;
    for (size_t i{ 0 }; i < strings.size(); i++)
    {
      if (i > 0)
        ss << delimiter;
      ss << fmt::format("{: ^{}}", strings[i], largest);
    }
    return ss.str();
  }

  // return a string representation of any vector containing z3::expr
  template <typename ExprVector>
  inline std::string join_ev(ExprVector const& c, bool align = false,
      const std::string delimiter = ", ")
  {
    // only join containers that can stream into stringstream
    if (c.size() == 0)
      return "";

    if (align)
      return join_ev_aligned(c, delimiter);

    std::stringstream ss;
    for (size_t i{ 0 }; i < c.size(); i++)
    {
      if (i > 0)
        ss << delimiter;
      ss << c[i].to_string();
    }
    return ss.str();
  }

  // expr_vector functions
  //
  z3::expr_vector mk_expr_vec(std::initializer_list<z3::expr> l);

  template <typename Container>
  z3::expr_vector mk_expr_vec(z3::context& ctx, Container container)
  {
    z3::ast_vector_tpl<typename Container::value_type> rv(ctx);
    for (typename Container::value_type const& e : container)
      rv.push_back(e);

    return rv;
  }

  // generate a new vector contain n copies of val
  template <typename T> z3::ast_vector_tpl<T> mk_vec(T const& val, size_t n)
  {
    z3::ast_vector_tpl<T> rv(val.ctx());
    for (size_t i{ 0 }; i < n; i++)
      rv.push_back(val);

    return rv;
  }

  // allocates new vector
  // by default, assignment and copy constructors copy a reference to an
  // internal vector. This constructs a deep copy, preventing the original
  // from being altered
  z3::expr_vector copy(z3::expr_vector const& v);

  template <typename T>
  z3::ast_vector_tpl<T> vec_add(
      z3::ast_vector_tpl<T> const& a, z3::ast_vector_tpl<T> const& b)
  {
    z3::ast_vector_tpl<T> rv(a.ctx());

    for (T const& e : a)
      rv.push_back(e);

    for (T const& e : b)
      rv.push_back(e);

    return rv;
  }

  // return a new z3::expr_vector,
  // made by transforming the z3::expr from an existing vector
  template <typename T, typename F>
  z3::ast_vector_tpl<T> transform(z3::ast_vector_tpl<T> const& vec, F func)
  {
    static_assert(
        std::is_same<typename std::invoke_result<F, T const&>::type, T>::value,
        "transform function must be of form func: T const& -> T");

    z3::ast_vector_tpl<T> rv(vec.ctx());
    for (T const& e : vec)
      rv.push_back(func(e));

    return rv;
  }

  // using CubeSet = std::set<ConstrainedCube, ccube_less>;
  using CubeSet = std::set<std::vector<z3::expr>, std_expr_vector_less>;

  namespace solver
  {
    struct Witness
    {
      std::vector<z3::expr> curr;
      std::vector<z3::expr> next;

      Witness(std::vector<z3::expr> const& c, std::vector<z3::expr> const& n);
      Witness(z3::expr_vector const& c, z3::expr_vector const& n);
    };
    // retrieve the current model in the solver as a cube
    // the resulting cube is sorted by lit_less
    // @ s has just completed a satisfiable check()
    z3::expr_vector get_witness(z3::solver const& s);
    std::vector<z3::expr> get_std_witness(z3::solver const& s);

    // retrieve the current unsat core in the solver as a cube
    // the resulting cube is sorted by lit_less
    // @ s has just completed an unsatisfiable check()
    z3::expr_vector get_core(z3::solver const& s);
    std::vector<z3::expr> get_std_core(z3::solver const& s);

    // perform a solver.check() and return the resulting witness
    std::optional<z3::expr_vector> check_witness(z3::solver& s);
    std::optional<z3::expr_vector> check_witness(
        z3::solver& s, z3::expr_vector const& assumptions);

    template <typename UnaryPredicate>
    std::vector<z3::expr> std_witness_st(z3::model const& m, UnaryPredicate p)
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
      order_lits(v);
      return v;
    }

    template <typename UnaryPredicate>
    z3::expr_vector witness_st(z3::solver const& s, UnaryPredicate p)
    {
      return convert(std_witness_st(s, p));
    }
  } // namespace solver

  namespace fixedpoint
  {
    std::vector<z3::expr> extract_trace_states(z3::fixedpoint& engine);
  } // namespace fixedpoint

  namespace tseytin
  {
    // convert e to cnf using z3's simplify and tseitin conversion tactics
    z3::expr_vector to_cnf_vec(z3::expr const& e);
    z3::expr to_cnf(z3::expr const& e);

    // add a tseytin encoded AND statement to "cnf"
    // c = a & b <=> (!a | !b | c) & (a | !c) & (b | !c)
    z3::expr add_and(z3::expr_vector& cnf, std::string const& name,
        z3::expr const& a, z3::expr const& b);

    // add a tseytin encoded OR statement to "cnf"
    // c = a | b <=> (a | b | !c) & (!a | c) & (!b | c)
    z3::expr add_or(z3::expr_vector& cnf, std::string const& name,
        z3::expr const& a, z3::expr const& b);

    // add a tseytin encoded IMPLIES statement to "cnf"
    // a => b <=> !a | b
    z3::expr add_implies(z3::expr_vector& cnf, std::string const& name,
        z3::expr const& a, z3::expr const& b);

    // add a tseytin encoded XOR statement to "cnf"
    // c = a ^ b <=> (!a | !b | !c) & (a | b | !c) & (a | !b | c) & (!a | b | c)
    z3::expr add_xor(z3::expr_vector& cnf, std::string const& name,
        z3::expr const& a, z3::expr const& b);

    // add a tseytin encoded XNOR statement to "cnf"
    // c = a ^ b <=> (!a | !b | c) & (a | b | c) & (a | !b | !c) & (!a | b | !c)
    z3::expr add_xnor(z3::expr_vector& cnf, std::string const& name,
        z3::expr const& a, z3::expr const& b);
  } // namespace tseytin
} // namespace z3ext
#endif // Z3_EXT
