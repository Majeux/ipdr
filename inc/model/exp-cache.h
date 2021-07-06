#ifndef EXP_CACHE
#define EXP_CACHE

#include <z3++.h>
#include <memory>
#include <string>
#include <unordered_map>

#include "string-extensions.h"

using std::shared_ptr;
using std::string;
using std::move;
using z3::context;
using z3::expr;
using z3::expr_vector;

class ExpressionCache
{
	private:
		enum class Encoding {UNKNOWN, LITERALS, EXPRESSIONS} encodes;
		shared_ptr<context> ctx;
		std::unordered_map<unsigned, int> literal_index;

		expr_vector literals;
		expr_vector literals_p;
		expr_vector neg_literals;
		expr_vector neg_literals_p;

	public:
		ExpressionCache(shared_ptr<context> c) :
			encodes(Encoding::UNKNOWN),
			ctx(c),
			literals(*c),
			literals_p(*c),
			neg_literals(*c),
			neg_literals_p(*c)
		{ }

		int indexof(const expr& e) const { return literal_index.at(e.id()); }

		//current state expressions
		expr operator()(const expr& e) const
		{
			int i = literal_index.at(e.id());
			return literals[i];
		}
		expr operator()(int i) const { return literals[i]; }

		//negated current state expressions
		expr neg(const expr& e) const
		{
			int i = literal_index.at(e.id());
			return neg_literals[i];
		}
		expr neg(int i) const { return neg_literals[i]; }

		//next state expressions
		expr p(const expr& e) const
		{
			int i = literal_index.at(e.id());
			return literals_p[i];
		}
		expr p(int i) const { return literals_p[i]; }

		//negated next state expressions
		expr negp(const expr& e) const
		{
			int i = literal_index.at(e.id());
			return neg_literals_p[i];
		}
		expr negp(int i) const { return neg_literals_p[i]; }

		//expose vectors for enumeration
		const expr_vector& lits() const { return literals; }
		const expr_vector& negs() const { return neg_literals; }
		const expr_vector& ps() const { return literals_p; }
		const expr_vector& neg_ps() const { return neg_literals_p; }
		const int size() const { return literals.size(); }
		

		void add_literal(const string& name)
		{
			if (encodes != Encoding::EXPRESSIONS)
				encodes = Encoding::LITERALS;

			expr lit = ctx->bool_const(name.c_str());
			expr lit_p = ctx->bool_const((name + ".p").c_str());
			expr neg_lit = !lit;
			expr neg_lit_p = !lit_p;

			literals.push_back(move(lit));
			literals_p.push_back(move(lit_p));
			neg_literals.push_back(move(neg_lit));
			neg_literals_p.push_back(move(neg_lit_p));

			literal_index.insert(std::make_pair(lit.id(), literals.size() - 1));
		}

		void add_expression(const expr& e, const ExpressionCache& cache)
		{
			assert(cache.encodes == Encoding::LITERALS);
			encodes = Encoding::EXPRESSIONS;
		}

		void print() const
		{
			std::cout << "Lits:          " << stringext::join(literals) << std::endl;
			std::cout << "Next Lits:     " << stringext::join(literals_p) << std::endl;
			std::cout << "Not Lits:      " << stringext::join(neg_literals) << std::endl;
			std::cout << "Not Next Lits: " << stringext::join(neg_literals_p) << std::endl;
		}
};

#endif
