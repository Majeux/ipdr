#ifndef PDR_ALG
#define PDR_ALG

#include "cli-parse.h"
#include "dag.h"
#include "frames.h"
#include "pdr-context.h"
#include "pdr-model.h"
#include "pebbling-model.h"
#include "pebbling-result.h"
#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "stats.h"
#include "vpdr.h"
#include "z3-ext.h"
#include "z3-pebbling-model.h"
#include "z3pdr.h"

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

  class PDR : public vPDR
  {
    friend class pebbling::IPDR;
    friend class peterson::IPDR;

   public:
    // Inherited from vPDR
    // vPDR(Context c, Logger& l)
    // get_ctx() -> Context const&

    PDR(my::cli::ArgumentList const& args, Context c, Logger& l, IModel& m);

    PdrResult run() override;
    void reset() override;

    Statistics& stats();
    void show_solver(std::ostream& out) const override;
    std::vector<std::string> trace_row(std::vector<z3::expr> const& v);
    int length_shortest_strategy() const;

   private:
    // inherited from vPDR
    // Context ctx
    // Logger& logger
    IModel& ts;

    spdlog::stopwatch timer;
    spdlog::stopwatch sub_timer;

    Frames frames; // sequence of candidates
    std::set<Obligation, std::less<Obligation>> obligations;

    struct HIFresult
    {
      int level;
      std::optional<std::vector<z3::expr>> core;
    };

    void print_model(z3::model const& m);
    // main algorithm
    PdrResult init();
    PdrResult iterate();
    PdrResult block(std::vector<z3::expr>&& cti, unsigned n);
    // generalization
    // todo return [n, cti ptr]
    HIFresult hif_(std::vector<z3::expr> const& cube, int min);
    HIFresult highest_inductive_frame(
        std::vector<z3::expr> const& cube, int min);
    void generalize(std::vector<z3::expr>& cube, int level);
    void MIC(std::vector<z3::expr>& cube, int level);
    void MICctg(std::vector<z3::expr>& cube, int level, unsigned depth);
    bool down(std::vector<z3::expr>& cube, int level);
    bool ctgdown(std::vector<z3::expr>& cube, int level, unsigned depth);
    // results
    void make_result(PdrResult& result);
    // to replace return value in run()
    // stores final logs, stats and result and returns its argument
    PdrResult finish(PdrResult&& rv);
    void store_frame_strings();

    // logging shorthands
    void log_start() const;
    void log_iteration();
    void log_cti(std::vector<z3::expr> const& cti, unsigned level);
    void log_propagation(unsigned level, double time);
    void log_top_obligation(size_t queue_size, unsigned top_level,
        std::vector<z3::expr> const& top);
    void log_pred(std::vector<z3::expr> const& p);
    void log_state_push(unsigned frame);
    void log_finish_state(std::vector<z3::expr> const& s);
    void log_obligation_done(std::string_view type, unsigned l, double time);
    void log_pdr_finish(PdrResult const& r, double final_time);
  };

  class vIPDR
  {
   public:
    vIPDR(my::cli::ArgumentList const& args, Context c, Logger& l, IModel& m)
        : alg(args, c, l, m)
    {
    }
    virtual ~vIPDR() {}

    PDR const& internal_alg() const { return alg; }

   protected: // usable by pdr and ipdr implementations
    PDR alg;
  };

  namespace pebbling
  {
    class IPDR : public vIPDR
    {
     public:
      IPDR(my::cli::ArgumentList const& args, Context c, Logger& l,
          PebblingModel& m);

      // runs the optimizer as dictated by the argument
      IpdrPebblingResult run(Tactic tactic);
      // runs the optimizer as dictated by the argument but with forced
      // control run (basic_reset only)
      IpdrPebblingResult control_run(Tactic tactic);
      IpdrPebblingResult relax(bool control);
      IpdrPebblingResult constrain(bool control);
      IpdrPebblingResult binary(bool control);
      IpdrPebblingResult relax_jump_test(unsigned start, int step);

     private:
      PebblingModel& ts; // same instance as the IModel in alg
      std::optional<unsigned> starting_pebbles;
      bool control_setting;

      void basic_reset(unsigned pebbles);
      // TODO: partial constraint strategy georg
      // If a cube cannot be propagated from a constraint p to p+1
      // Add "cube \land __le_p__" where "atmost(p) \land atmost(p)' <=>
      // __le_p__" This cube was already inductive under this lower constraint,
      // so now the exact same ctis are not rediscovered
      //  note: subsumption still works as normal? a more specific (subcube)
      //  subsumes the larger
      void relax_reset(unsigned pebbles);
      void relax_reset_constrained(unsigned pebbles);
      std::optional<size_t> constrain_reset(unsigned pebbles);
    }; // class Optimizer
  }    // namespace pebbling

  namespace peterson
  {
    class IPDR : public vIPDR
    {
     public:
      IPDR(my::cli::ArgumentList const& args, Context c, Logger& l,
          PetersonModel& m);

      // runs the optimizer as dictated by the argument
      IpdrPetersonResult run(Tactic tactic, std::optional<unsigned> processes);
      // runs the optimizer as dictated by the argument but with forced
      // experiment_control (basic_reset only)
      IpdrPetersonResult control_run(Tactic tactic, unsigned processes);
      IpdrPetersonResult relax(unsigned processes, bool control);
      IpdrPetersonResult relax_jump_test(unsigned start, int step);

      // PDR const& internal_alg() const; from vIPDR
     private:
      // PDR alg; from vIPDR
      PetersonModel& ts; // same instance as the IModel in alg
      bool control_setting;

      void basic_reset(unsigned processes);
      void relax_reset(unsigned processes);
    }; // class Optimizer
  }    // namespace peterson
} // namespace pdr
#endif // PDR_ALG
