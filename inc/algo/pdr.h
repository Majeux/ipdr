#ifndef PDR_ALG
#define PDR_ALG

#include "frames.h"
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
#define MIN_ORDERING(T) T, std::vector<T>, std::greater<T> //type arguments for ascending priority queue

namespace pdr 
{
	struct State 
	{
		expr_vector cube;
		std::shared_ptr<State> prev; //store predecessor for trace

		State(const expr_vector& e) : cube(e), prev(std::shared_ptr<State>()) { }
		State(const expr_vector& e, std::shared_ptr<State> s) : cube(e), prev(s) { }
		//move constructors
		State(expr_vector&& e) : cube(std::move(e)), prev(std::shared_ptr<State>()) { }
		State(expr_vector&& e, std::shared_ptr<State> s) : cube(std::move(e)), prev(s) { }
	};

	struct Obligation
	{
		unsigned level;
		std::shared_ptr<State> state;
		unsigned depth;

		Obligation(unsigned k, expr_vector&& cube, unsigned d) : 
			level(k), state( std::make_shared<State>(std::move(cube)) ), depth(d) { }

		Obligation(unsigned k, const std::shared_ptr<State>& s, unsigned d) : level(k), state(s), depth(d) { }

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
			bool delta; //use a delta encoding for the frames

			Logger logger;
			spdlog::stopwatch timer;
			spdlog::stopwatch sub_timer;

			std::shared_ptr<State> bad;

			unsigned k = 0;
			Frames frames;

			const unsigned mic_retries = 3; //if mic fails to reduce a clause c this many times, take c

			void print_model(const z3::model& m);
			//main loops
			bool init();
			bool iterate();
			bool iterate_short();
			bool block(expr_vector& counter, unsigned o_level, unsigned level);
			bool block_short(expr_vector& counter, unsigned o_level, unsigned level);
			void remove_state(expr_vector& cube, unsigned level);
			bool propagate(unsigned level, bool repeat = false);
			bool fat_propagate(unsigned level, bool repeat = false);
			bool delta_propagate(unsigned level, bool repeat = false);
			bool repropagate();
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
			void store_frame_strings();

		public:
			// bool dynamic_cardinality = true;
			bool dynamic_cardinality = false;
			string frames_string = "";
			string solvers_string = "";
			Statistics stats;

			PDR(PDRModel& m, bool d);
			void reset();
			bool run(bool dynamic = false);
			void show_results(std::ostream& out = std::cout) const;
			void decrement(int x);
	};
}
#endif //PDR_ALG
