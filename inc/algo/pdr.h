#ifndef PDR_ALG
#define PDR_ALG

#include "_logging.h"
#include "frames.h"
#include "pdr-model.h"
#include "pdr-context.h"
#include "result.h"
#include "stats.h"
#include "z3-ext.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <ostream>
#include <queue>
#include <spdlog/stopwatch.h>
#include <string>
#include <vector>
#include <z3++.h>
#include <z3_fpa.h>

// type arguments for ascending priority queue
#define MIN_ORDERING(T) T, std::vector<T>, std::greater<T>

namespace pdr
{
  class PDR
  {
   private:
    context& ctx;

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;
    Logger& logger;

    unsigned k = 0;
    Frames frames;

    Results& results;
    int shortest_strategy;

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
    bool down(std::vector<z3::expr>& cube, int level);
    // results
    void store_result();
    void show_trace(const std::shared_ptr<State> trace_root,
                    std::ostream& out) const;
    bool finish(bool);
    void store_frame_strings();

    // logging shorthands 
    void log_start() const;
    void log_iteration();
    void log_cti(const z3::expr_vector& cti);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(size_t queue_size, unsigned top_level,
                            const z3::expr_vector& top);
    void log_pred(const z3::expr_vector& p);
    void log_state_push(unsigned frame, const z3::expr_vector& p);
    void log_finish(const z3::expr_vector& s);
    void log_obligation(std::string_view type, unsigned l, double time);

   public:
    // bool dynamic_cardinality = true;
    bool dynamic_cardinality   = false;
    std::string frames_string  = "";
    std::string solvers_string = "";

    PDR(context& c, Logger& l, Results& r);
    // prepare PDR for new run. discards old trace
    void reset();

    // execute the PDR algorithm 
    // returns true if the property is invariant
    // returns false if there is a trace to a violation
    bool run(bool optimize = false);
    void show_solver(std::ostream& out) const;
    void show_results(std::ostream& out) const;

    // reduces the max pebbles of the model to 1 lower than the previous
    // strategy length. returns true if the is already proven invariant by this.
    // returns false if this remains to be verified.
    bool decrement(bool reuse = false);
    bool increment_strategy(std::ofstream& strategy, std::ofstream& solver_dump);

    Statistics& stats();
    int length_shortest_strategy() const;
  };
} // namespace pdr
#endif // PDR_ALG
