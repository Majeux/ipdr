#ifndef PDR_ALG
#define PDR_ALG

#include "_logging.h"
#include "frames.h"
#include "pdr-context.h"
#include "pdr-model.h"
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

namespace pdr
{
  class PDR
  {
   private:
    Context& ctx;

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;
    Logger& logger;

    Frames frames;
    std::set<Obligation, std::less<Obligation>> obligations;

    Results results;
    int shortest_strategy;

    // if mic fails to reduce a clause c this many times, take c
    const unsigned mic_retries = 3;

    void print_model(const z3::model& m);
    // main loops
    bool init();
    bool iterate();
    bool iterate_short();
    bool block(z3::expr_vector cti, unsigned n);
    bool block_short(z3::expr_vector& counter, unsigned o_level,
                     unsigned level);
    // generalization
    // todo return [n, cti ptr]
    int hif_(const z3::expr_vector& cube, int min, int max);
    std::tuple<int, z3::expr_vector>
        highest_inductive_frame(const z3::expr_vector& cube, int min, int max);
    z3::expr_vector generalize(const z3::expr_vector& cube, int level);
    z3::expr_vector MIC(const z3::expr_vector& cube, int level);
    bool down(std::vector<z3::expr>& cube, int level);
    // results
    void store_result();
    void store_result2();
    void show_trace(const std::shared_ptr<State> trace_root,
                    std::ostream& out) const;
    // to replace return value in run()
    // stores final logs, stats and result and returns its argument
    bool finish(bool);
    void store_frame_strings();

    // logging shorthands
    void log_start() const;
    void log_iteration();
    void log_cti(const z3::expr_vector& cti, unsigned level);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(size_t queue_size, unsigned top_level,
                            const z3::expr_vector& top);
    void log_pred(const z3::expr_vector& p);
    void log_state_push(unsigned frame);
    void log_finish(const z3::expr_vector& s);
    void log_obligation_done(std::string_view type, unsigned l, double time);

   public:
    PDR(Context& c, Logger& l);
    // prepare PDR for new run. discards old trace
    void reset();

    // execute the PDR algorithm
    // returns true if the property is invariant
    // returns false if there is a trace to a violation
    bool run(Tactic pdr_type = Tactic::basic);
    void show_solver(std::ostream& out) const;
    void show_results(std::ostream& out) const;
    std::vector<std::string> trace_row(const z3::expr_vector& v);

    // reduces the max pebbles of the model to 1 lower than the previous
    // strategy length. returns true if the is already proven invariant by this.
    // returns false if this remains to be verified.
    bool decrement(bool reuse = false);
    // Start at max pebbles and decrement until (at the lowest) final pebbles
    bool dec_tactic(std::ofstream& strategy, std::ofstream& solver_dump);
    // start at final pebbles and increment until (at the most) max pebbles
    bool inc_tactic(std::ofstream& strategy, std::ofstream& solver_dump);
    bool inc_jump_test(int start, int step, std::ofstream& strategy,
                       std::ofstream& solver_dump);

    Statistics& stats();
    int length_shortest_strategy() const;
  };
} // namespace pdr
#endif // PDR_ALG
