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
  namespace pebbling
  {
    class Optimizer;
  }

  class PDR
  {
    friend class pebbling::Optimizer;

   private:
    Context& ctx;

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;
    Logger& logger;

    Frames frames;
    std::set<Obligation, std::less<Obligation>> obligations;

    unsigned shortest_strategy;

    // if mic fails to reduce a clause c this many times, take c
    const unsigned mic_retries = UINT_MAX;

    void print_model(const z3::model& m);
    // main algorithm
    Result _run();
    Result init();
    Result iterate();
    Result block(z3::expr_vector cti, unsigned n);
    bool iterate_short();
    bool block_short(
        z3::expr_vector& counter, unsigned o_level, unsigned level);
    // generalization
    // todo return [n, cti ptr]
    int hif_(const z3::expr_vector& cube, int min);
    std::tuple<int, z3::expr_vector> highest_inductive_frame(
        const z3::expr_vector& cube, int min);
    z3::expr_vector generalize(const z3::expr_vector& cube, int level);
    z3::expr_vector MIC(const z3::expr_vector& cube, int level);
    bool down(std::vector<z3::expr>& cube, int level);
    // results
    void make_result(Result& result);
    // to replace return value in run()
    // stores final logs, stats and result and returns its argument
    Result finish(Result&& rv);
    void store_frame_strings();
    std::string constraint_str() const;

    // logging shorthands
    void log_start() const;
    void log_iteration();
    void log_cti(const z3::expr_vector& cti, unsigned level);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(
        size_t queue_size, unsigned top_level, const z3::expr_vector& top);
    void log_pred(const z3::expr_vector& p);
    void log_state_push(unsigned frame);
    void log_finish(const z3::expr_vector& s);
    void log_obligation_done(std::string_view type, unsigned l, double time);

   public:
    PDR(Context& c, Logger& l);
    // prepare PDR for new run. discards old trace
    void reset();
    const Context& get_ctx() const;
    Context& get_ctx();

    // execute the PDR algorithm using the model and property in the context
    // returns true if the property is invariant
    // returns false if there is a trace to a violation
    // max_pebbles: {} gives Frames no constraint
    // any value constrains maximum pebbled literals to the number
    Result run(Tactic pdr_type              = Tactic::basic,
        std::optional<unsigned> max_pebbles = {});
    // a run loosening the constraint, assuming a previous run was completed
    Result decrement_run(unsigned max_pebbles);
    Result increment_run(unsigned max_pebbles);

    // constrain the given model to allow only 'x' literals marked
    void reconstrain(unsigned x);
    // reduces the max pebbles of the model to 1 lower than the previous
    // strategy length. returns true if the is already proven invariant by this.
    // returns false if this remains to be verified.
    bool decrement(bool reuse = false);

    // optimization tactics
    //
    // Start at max pebbles and decrement until (at the lowest) final pebbles
    bool dec_tactic(std::ofstream& strategy, std::ofstream& solver_dump);
    // start at final pebbles and increment until (at the most) max pebbles
    bool inc_tactic(std::ofstream& strategy, std::ofstream& solver_dump);
    bool inc_jump_test(unsigned start, int step, std::ofstream& strategy,
        std::ofstream& solver_dump);

    Statistics& stats();
    void show_solver(std::ostream& out) const;
    std::vector<std::string> trace_row(const z3::expr_vector& v);
    int length_shortest_strategy() const;
  };

  namespace pebbling
  {
    class Optimizer
    {
     private:
      PDR alg;

     public:
      Results latest_results;

      Optimizer(PDR&& a);
      Optimizer(Context& c, Logger& l);

      // runs the optimizer as dictated by the argument
      std::optional<unsigned> run(my::cli::ArgumentList args);
      std::optional<unsigned> increment(bool control);
      std::optional<unsigned> decrement(bool control);
      void inc_jump_test(unsigned start, int step);

      void dump_solver(std::ofstream& out) const;
    }; // class Optimizer
  }    // namespace pebbling
} // namespace pdr
#endif // PDR_ALG
