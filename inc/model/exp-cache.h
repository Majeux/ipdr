#ifndef EXP_CACHE
#define EXP_CACHE

#include <z3++.h>
#include <cassert>
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
		bool finished = false; //add no new expressions after finish() has been called
		enum class Encoding {UNKNOWN, LITERALS, EXPRESSIONS} encodes;
		shared_ptr<context> ctx;
		std::unordered_map<unsigned, int> literal_index;

		expr_vector current;
		expr_vector next;

	public:
		ExpressionCache(shared_ptr<context> c) :
			encodes(Encoding::UNKNOWN),
			ctx(c),
			current(*c),
			next(*c)
		{ }

		int indexof(const expr& e) const { return literal_index.at(e.id()); }

		//current state expressions
		expr operator()(const expr& e) const
		{
			int i = literal_index.at(e.id());
			return current[i];
		}
		expr operator()(int i) const { return current[i]; }

		//next state expressions
		expr p(const expr& e) const
		{
			int i = literal_index.at(e.id());
			return next[i];
		}
		expr p(int i) const { return next[i]; }
		
		//converts a vector of literals into a vector of literals in the next state
		//assumes vec is a vector of consts in current
		expr_vector p(const expr_vector& vec) const
		{
			expr_vector vec_next(*ctx);
			for (const expr& e : vec)
				vec_next.push_back(next[e.id()]);
			return vec_next;
		}

		//expose vectors for enumeration
		const expr_vector& currents() const { return current; }
		const expr_vector& nexts() const { return next; }
		int size() const { return current.size(); }
		

		void add_literal(const string& name)
		{
			assert(!finished);
			if (encodes != Encoding::EXPRESSIONS)
				encodes = Encoding::LITERALS;

			expr lit = ctx->bool_const(name.c_str());
			expr lit_p = ctx->bool_const((name + ".p").c_str());

			current.push_back(move(lit));
			next.push_back(move(lit_p));

			literal_index.insert(std::make_pair(lit.id(), current.size() - 1));
		}

		void add_expression(expr e, const ExpressionCache& cache)
		{
			assert(!finished);
			assert(cache.encodes == Encoding::LITERALS);
			encodes = Encoding::EXPRESSIONS;

			current.push_back(e);
			expr e_next = e.substitute(cache.currents(), cache.nexts());
			next.push_back(e_next);
		}

		void finish() { finished = true; }

		void print() const
		{
			std::cout << "Lits:      " << stringext::join(current) << std::endl;
			std::cout << "Next Lits: " << stringext::join(next) << std::endl;
		}
};

#endif
