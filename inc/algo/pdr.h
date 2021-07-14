#ifndef PDR_ALG
#define PDR_ALG
// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <ostream>
#include <queue>
#include <memory>
#include <vector>
#include <z3++.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include "frame.h"
#include "pdr-model.h"

using std::vector;
using std::unique_ptr;
using std::shared_ptr;
using z3::context;
using z3::solver;

#define SEP "--------------------"
#define SEP2 "===================="
#define SEP3 "####################"
#define TAB std::string(log_indent, '\t')
#define MIN_ORDERING(T) T, std::vector<T>, std::greater<T> //type arguments for ascending priority queue

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

	Obligation(unsigned k, expr_vector&& cube) : 
		level(k), 
		state( std::make_shared<State>(std::move(cube)) ) { }

	Obligation(unsigned k, const shared_ptr<State>& s) : level(k), state(s) { }

	// bool operator<(const Obligation& o) const { return this->level < o.level; }
	bool operator>(const Obligation& o) const { return this->level > o.level; }
};

class PDR 
{
	private:
		shared_ptr<context> ctx;
		const PDRModel& model;

		shared_ptr<spdlog::logger> log;
		unsigned log_indent = 0;
		spdlog::stopwatch timer;
		spdlog::stopwatch sub_timer;
		shared_ptr<State> bad;

		vector<unique_ptr<Frame>> frames;
		solver init_solver;

		const unsigned mic_retries = 3; //if mic fails to reduce a clause c this many times, take c

		Frame* make_frame(int level);
		void print_model(const z3::model& m);
		//main loops
		bool init();
		bool iterate();
		bool block(std::priority_queue<MIN_ORDERING(Obligation)> obligations, unsigned level);
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

	public:
		PDR(shared_ptr<context> c, const PDRModel& m, bool log);
		bool run();
		void show_results(std::ostream& out = std::cout) const;
};

#endif //PDR_ALG
