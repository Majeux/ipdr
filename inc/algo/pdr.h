#ifndef PDR_ALG
#define PDR_ALG

#include <ostream>
#include <queue>
#include <memory>
#include <vector>
#include <z3++.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

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


struct Obligation
{
	unsigned level;
	expr_vector cube;

	Obligation(unsigned k, const expr_vector& e) : level(k), cube(e) { }

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

		vector<unique_ptr<Frame>> frames;
		solver init_solver;

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
		expr_vector generalize(const expr_vector& cube, int level);
		expr_vector MIC(const expr_vector& cube, int level);
		bool down(vector<expr>& cube, int level);
		//results
		void show_trace(std::ostream& out) const;

	public:
		PDR(shared_ptr<context> c, const PDRModel& m);
		void run();
		void show_results(std::ostream& out = std::cout) const;
};

#endif //PDR_ALG
