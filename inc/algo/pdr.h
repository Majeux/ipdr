#ifndef PDR_ALG
#define PDR_ALG

#include "frame.h"
#include "pdr-model.h"
#include "stats.h"
#include "logging.h"
#include "z3-ext.h"

#include <ostream>
#include <queue>
#include <memory>
#include <vector>
#include <z3++.h>
#include <spdlog/stopwatch.h>

#define SEP "--------------------"
#define SEP2 "===================="
#define SEP3 "####################"
#define TAB std::string(log_indent, '\t')
#define MIN_ORDERING(T) T, std::vector<T>, std::greater<T> //type arguments for ascending priority queue

namespace pdr 
{
	using std::vector;
	using std::unique_ptr;
	using std::shared_ptr;
	using z3::context;
	using z3::solver;

	struct State 
	{
		expr_vector cube;
		shared_ptr<State> prev; //store predecessor for trace

		State(const expr_vector& e) : cube(e), prev(shared_ptr<State>()) { }
		State(const expr_vector& e, shared_ptr<State> s) : cube(e), prev(s) { }
		//move constructors
		State(expr_vector&& e) : cube(std::move(e)), prev(shared_ptr<State>()) { }
		State(expr_vector&& e, shared_ptr<State> s) : cube(std::move(e)), prev(s) { }
	};

	struct Obligation
	{
		unsigned level;
		shared_ptr<State> state;
		unsigned depth;

		Obligation(unsigned k, expr_vector&& cube, unsigned d) : 
			level(k), state( std::make_shared<State>(std::move(cube)) ), depth(d) { }

		Obligation(unsigned k, const shared_ptr<State>& s, unsigned d) : level(k), state(s), depth(d) { }

		// bool operator<(const Obligation& o) const { return this->level < o.level; }
		bool operator<(const Obligation& o) const 
		{ 
			if (this->level < o.level) return true;
			if (this->level > o.level) return false;
			if (this->depth < o.depth) return true;
			if (this->depth > o.depth) return false;

			return z3ext::expr_vector_less()(this->state->cube, o.state->cube); 
		}
	};

	class PDR 
	{
		private:
			context& ctx;
			PDRModel& model;

			shared_ptr<spdlog::logger> log;
			unsigned log_indent = 0;
			spdlog::stopwatch timer;
			spdlog::stopwatch sub_timer;

			shared_ptr<State> bad;

			vector<unique_ptr<Frame>> old_frames;
			vector<unique_ptr<Frame>> frames;
			solver init_solver;

			const unsigned mic_retries = 3; //if mic fails to reduce a clause c this many times, take c

			Frame* make_frame(unsigned level);
			void extend_frames(unsigned level);
			void print_model(const z3::model& m);
			//main loops
			bool init();
			bool iterate();
			bool iterate_short();
			bool block(expr_vector& counter, unsigned o_level, unsigned level);
			bool block_short(expr_vector& counter, unsigned o_level, unsigned level);
			void remove_state(expr_vector& cube, int level);
			bool propagate(unsigned level);
			//generalization
			int highest_inductive_frame(const expr_vector& cube, int min, int max);
			int highest_inductive_frame(const expr_vector& cube, int min, int max, expr_vector& core);
			expr_vector generalize(const expr_vector& cube, int level);
			expr_vector MIC(const expr_vector& cube, int level);
			bool down(vector<expr>& cube, int level);
			//results
			void show_trace(std::ostream& out) const;
			bool finish(bool);
			void store_frames();

		public:
			bool dynamic_cardinality = true;
			Statistics stats;

			PDR(PDRModel& m);
			bool run();
			void show_results(std::ostream& out = std::cout) const;
			void decrement(unsigned x);
	};
}
#endif //PDR_ALG
