#ifndef PDR_ALG
#define PDR_ALG

#include "_logging.h"
#include "frames.h"
#include "pdr-model.h"
#include "stats.h"
#include "z3-ext.h"

#include <memory>
#include <ostream>
#include <queue>
#include <spdlog/stopwatch.h>
#include <vector>
#include <z3++.h>

// type arguments for ascending priority queue
#define MIN_ORDERING(T) T, std::vector<T>, std::greater<T>

namespace pdr
{
  struct State
  {
    z3::expr_vector cube;
    std::shared_ptr<State> prev; // store predecessor for trace

    State(const z3::expr_vector& e) : cube(e), prev(std::shared_ptr<State>()) {}
    State(const z3::expr_vector& e, std::shared_ptr<State> s) : cube(e), prev(s)
    {
    }
    // move constructors
    State(z3::expr_vector&& e)
        : cube(std::move(e)), prev(std::shared_ptr<State>())
    {
    }
    State(z3::expr_vector&& e, std::shared_ptr<State> s)
        : cube(std::move(e)), prev(s)
    {
    }
  };

  struct Obligation
  {
    unsigned level;
    std::shared_ptr<State> state;
    unsigned depth;

    Obligation(unsigned k, z3::expr_vector&& cube, unsigned d)
        : level(k), state(std::make_shared<State>(std::move(cube))), depth(d)
    {
    }

    Obligation(unsigned k, const std::shared_ptr<State>& s, unsigned d)
        : level(k), state(s), depth(d)
    {
    }

    // bool operator<(const Obligation& o) const { return this->level <
    // o.level; }
    bool operator<(const Obligation& o) const
    {
      if (this->level < o.level)
        return true;
      if (this->level > o.level)
        return false;
      if (this->depth < o.depth)
        return true;
      if (this->depth > o.depth)
        return false;

      return z3ext::expr_vector_less()(this->state->cube, o.state->cube);
    }
  };

  struct PDResult
  {
    std::shared_ptr<State> trace;
    std::string trace_string;
    unsigned pebbles_used;
    size_t invariant_index;
    double total_time;
  };

  class PDR
  {
  private:
    z3::context& ctx;
    PDRModel& model;
    bool delta; // use a delta encoding for the frames

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;
    Logger logger;

    PDResult result;

    unsigned k = 0;
    Frames frames;

    // if mic fails to reduce a clause c this many times, take c
    const unsigned mic_retries = 3;

    void print_model(const z3::model& m);
    // main loops
    bool init();
    bool iterate();
    bool iterate_short();
    bool block(z3::expr_vector& counter, unsigned o_level, unsigned level);
    bool block_short(z3::expr_vector& counter, unsigned o_level,
                     unsigned level);
    // generalization
    int highest_inductive_frame(const z3::expr_vector& cube, int min, int max);
    int highest_inductive_frame(const z3::expr_vector& cube, int min, int max,
                                z3::expr_vector& core);
    z3::expr_vector generalize(const z3::expr_vector& cube, int level);
    z3::expr_vector MIC(const z3::expr_vector& cube, int level);
    bool down(vector<z3::expr>& cube, int level);
    // results
    void store_result();
    void show_trace(std::ostream& out) const;
    bool finish(bool);
    void store_frame_strings();

    void log_and_show(const std::string& str, std::ostream& out = std::cout);
    void log_iteration();
    void log_cti(const z3::expr_vector& cti);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(size_t queue_size, unsigned top_level,
                            const z3::expr_vector& top);
    void log_pred(const z3::expr_vector& p);
    void log_state_push(unsigned frame, const z3::expr_vector& p);
    void log_finish(const z3::expr_vector& s);
    void log_obligation(const std::string& type, unsigned l, double time);

  public:
    // bool dynamic_cardinality = true;
    bool dynamic_cardinality = false;
    string frames_string = "";
    string solvers_string = "";

    PDR(PDRModel& m, bool d, const std::string& log_file);
    void reset();
    bool run(bool optimize = false);
    void show_results(std::ostream& out = std::cout) const;
    void decrement(int x);

    Statistics& stats();
  };
} // namespace pdr
#endif // PDR_ALG
