#ifndef EXP_CACHE
#define EXP_CACHE

#include <vector>
#include <z3++.h>
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

#include "string-ext.h"

using std::shared_ptr;
using std::string;
using std::move;
using z3::context;
using z3::expr;
using z3::expr_vector;
using str::extensions::join;

class ExpressionCache
{
	private:
		bool finished = false; //add no new expressions after finish() has been called
		enum class Encoding {UNKNOWN, LITERALS, EXPRESSIONS} encodes;
		shared_ptr<context> ctx;
		std::unordered_map<unsigned, int> literal_index;
		std::unordered_map<unsigned, int> literal_index_p;

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
		//checks if e is an atom in current. fails if e is not an atom (const)
		bool atom_is_current(const expr& e) const 
		{
			assert(e.is_const());
			return literal_index.find(e.id()) != literal_index.end();
		}
		//checks if e is a literal in current. fails if e is not a literal (atom/!atom)
		bool literal_is_current(const expr& e) const 
		{
			if(e.is_not())
				return literal_index.find(e.arg(0).id()) != literal_index.end();
			return atom_is_current(e);
		}

		bool literal_is_next(const expr& e) const 
		{
			if(e.is_not())
				return literal_index_p.find(e.arg(0).id()) != literal_index.end();
			assert(e.is_const());
			return literal_index_p.find(e.id()) != literal_index.end();
		}

		//converts a literal in the next state to a literal in the current state
		expr operator()(const expr& e) const
		{
			int index;
			if(e.is_not())
			{
				index = literal_index_p.at(e.arg(0).id());
				return !current[index];
			}
			assert(e.is_const());
			index = literal_index_p.at(e.id());
			return current[index];
		}
		expr operator()(int index) const { return current[index]; }

		expr_vector operator()(const expr_vector& vec) const
		{
			expr_vector vec_now(*ctx);
			for (const expr& e : vec)
				vec_now.push_back(operator()(e));
			return vec_now;
		}

		//next state expressions
		expr p(const expr& e) const
		{
			if (e.is_not())
			{
				int index = literal_index.at(e.arg(0).id());
				return !next[index];
			}
			assert(e.is_const());

			int index = literal_index.at(e.id());
			return next[index];
		}
		expr p(int index) const { return next[index]; }
		
		//converts a vector of literals into a vector of literals in the next state
		//assumes vec is a vector of consts in current
		expr_vector p(const expr_vector& vec) const
		{
			expr_vector vec_next(*ctx);
			for (const expr& e : vec)
				vec_next.push_back(p(e));
			return vec_next;
		}
		expr_vector p(const std::vector<expr>& vec) const
		{
			expr_vector vec_next(*ctx);
			for (const expr& e : vec)
				vec_next.push_back(p(e));
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
			literal_index_p.insert(std::make_pair(lit_p.id(), current.size() - 1));
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
			std::cout << "Lits:      " << join(current) << std::endl;
			std::cout << "Next Lits: " << join(next) << std::endl;
		}
};

#endif
