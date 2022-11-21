#ifndef PDR_ALG
#define PDR_ALG

#include "frames.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "pebbling-model.h"
#include "pebbling-result.h"
#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "stats.h"
#include "z3-ext.h"

#include <climits>
#include <cstdint>
#include <memory>
#include <optional>
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
    class IPDR;
  }
  namespace peterson
  {
    class IPDR;
  }

  class PDR
  {
    friend class pebbling::IPDR;
    friend class peterson::IPDR;

   public:
    PDR(Context& c, IModel& m, Logger& l);

    // prepare PDR for new run. discards old trace
    void reset();
    PdrResult run();

    Context const& get_ctx() const;
    Context& get_ctx();

    Statistics& stats();
    void show_solver(std::ostream& out) const;
    std::vector<std::string> trace_row(z3::expr_vector const& v);
    int length_shortest_strategy() const;

   private:
    Context& ctx;

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;
    Logger& log;

    Frames frames; // sequence of candidates
    std::set<Obligation, std::less<Obligation>> obligations;

    // if mic fails to reduce a clause c this many times, take c
    const unsigned mic_retries = UINT_MAX;

    struct HIFresult
    {
      int level;
      std::optional<z3::expr_vector> core;
    };

    void print_model(z3::model const& m);
    // main algorithm
    PdrResult init();
    PdrResult iterate();
    PdrResult block(z3::expr_vector cti, unsigned n);
    PdrResult iterate_short();
    PdrResult block_short(z3::expr_vector&& counter, unsigned n);
    // generalization
    // todo return [n, cti ptr]
    HIFresult hif_(z3::expr_vector const& cube, int min);
    HIFresult highest_inductive_frame(z3::expr_vector const& cube, int min);
    z3::expr_vector generalize(z3::expr_vector const& cube, int level);
    z3::expr_vector MIC(z3::expr_vector const& cube, int level);
    bool down(std::vector<z3::expr>& cube, int level);
    // results
    void make_result(PdrResult& result);
    // to replace return value in run()
    // stores final logs, stats and result and returns its argument
    PdrResult finish(PdrResult&& rv);
    void store_frame_strings();

    // logging shorthands
    void log_start() const;
    void log_iteration();
    void log_cti(z3::expr_vector const& cti, unsigned level);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(
        size_t queue_size, unsigned top_level, z3::expr_vector const& top);
    void log_pred(z3::expr_vector const& p);
    void log_state_push(unsigned frame);
    void log_finish(z3::expr_vector const& s);
    void log_obligation_done(std::string_view type, unsigned l, double time);
  };

  namespace pebbling
  {
    class IPDR
    {
     public:
      IPDR(Context& c, PebblingModel& m, my::cli::ArgumentList const& args,
          Logger& l);

      // runs the optimizer as dictated by the argument
      PebblingResult run(Tactic tactic, bool control = false);
      // runs the optimizer as dictated by the argument but with forced
      // experiment_control
      PebblingResult control_run(Tactic tactic);
      PebblingResult relax(bool control);
      PebblingResult constrain(bool control);
      PebblingResult relax_jump_test(unsigned start, int step);

      PDR const& internal_alg() const;

     private:
      PDR alg;
      PebblingModel& model; // same instance as the IModel in alg
      std::optional<unsigned> starting_value;

      void basic_reset(unsigned pebbles);
      void relax_reset(unsigned pebbles);
      std::optional<size_t> constrain_reset(unsigned pebbles);
    }; // class Optimizer
  }    // namespace pebbling

  namespace peterson
  {
    class IPDR
    {
     public:
      IPDR(Context& c, PetersonModel& m, my::cli::ArgumentList const& args,
          Logger& l);

      // runs the optimizer as dictated by the argument
      PetersonResult run(
          Tactic tactic, unsigned processes, bool control = false);
      // runs the optimizer as dictated by the argument but with forced
      // experiment_control
      PetersonResult control_run(Tactic tactic, unsigned processes);
      PetersonResult relax(unsigned processes, bool control);
      PetersonResult relax_jump_test(unsigned start, int step);

      PDR const& internal_alg() const;

     private:
      PDR alg;
      PetersonModel& model; // same instance as the IModel in alg

      void basic_reset(unsigned processes);
      void relax_reset(unsigned processes);
    }; // class Optimizer
  }    // namespace peterson
} // namespace pdr
#endif // PDR_ALG
