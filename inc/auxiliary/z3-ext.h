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

    inline expr minus(const expr& e) { return e.is_not() ? e.arg(0) : !e; }

	// !allocates new vector
    // by default, assignment and copy constructors copy a reference to an
    // internal vector. This constructs a deep copy, preventing the original
    // from being altered
    inline expr_vector copy(const expr_vector& v)
    {
        z3::expr_vector new_v(v.ctx());
        for (const z3::expr& e : v)
            new_v.push_back(e);

        return new_v;
    }

	// !allocates new vector
    inline expr_vector negate(const expr_vector& lits)
    {
        expr_vector negated(lits.ctx());
        for (const expr& e : lits)
            negated.push_back(minus(e));
        return negated;
    }

	// !allocates new vector
    inline expr_vector negate(const vector<expr>& lits)
    {
        expr_vector negated(lits[0].ctx());
        for (const expr& e : lits)
            negated.push_back(minus(e));
        return negated;
    }

	// !allocates new vector
    inline expr_vector convert(const vector<expr>& vec)
    {
		assert(vec.size() > 0);
        expr_vector converted(vec[0].ctx());
        for (const expr& e : vec)
            converted.push_back(std::move(e));
        return converted;
    }

	// !allocates new vector
    inline expr_vector convert(vector<expr>&& vec)
    {
		assert(vec.size() > 0);
        expr_vector converted(vec[0].ctx());
        for (const expr& e : vec)
            converted.push_back(std::move(e));
        return converted;
    }

    inline vector<expr> convert(const expr_vector& vec)
    {
        vector<expr> converted;
        converted.reserve(vec.size());
        for (const expr& e : vec)
            converted.push_back(e);
        return converted;
    }

	// !allocates new vector
    inline expr_vector args(const expr& e)
    {
        expr_vector vec(e.ctx());
        for (unsigned i = 0; i < e.num_args(); i++)
            vec.push_back(e.arg(i));
        return vec;
    }

    template <typename ExprVector>
    inline string join_expr_vec(const ExprVector& c, bool align = true,
                                const string delimiter = ", ")
    {
        // only join containers that can stream into stringstream
        if (c.size() == 0)
            return "";

        vector<string> strings;
        strings.reserve(c.size());
        std::transform(c.begin(), c.end(), std::back_inserter(strings),
                       [](const z3::expr& i) { return i.to_string(); });

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

    // z3::expr comparator
    struct expr_less
    {
        bool operator()(const z3::expr& l, const z3::expr& r) const
        {
            return l.id() < r.id();
        };
    };

    inline void sort(expr_vector& v)
    {
        vector<expr> std_vec = convert(v);
        std::sort(std_vec.begin(), std_vec.end(), expr_less());
        v = convert(std::move(std_vec));
    }

    // returns true if l c= r
    // assumes l and r are in sorted order
    inline bool subsumes(const expr_vector& l, const expr_vector& r)
    {
        // return false;
        if (l.size() > r.size())
            return false;

        return std::includes(r.begin(), r.end(), l.begin(), l.end(),
                             expr_less());
    }

    struct expr_vector_less
    {
        bool operator()(const expr_vector& l, const expr_vector& r) const
        {
            auto l_it = l.begin();
            auto r_it = r.begin();

            for (; l_it != l.end() && r_it != r.end(); l_it++, r_it++)
            {
                if ((*l_it).id() < (*r_it).id())
                    return true;
                if ((*l_it).id() > (*r_it).id())
                    return false;
            }
            // all elements equal to a point
            return l.size() < r.size();
        }
    };

} // namespace z3ext
#endif // Z3_EXT
