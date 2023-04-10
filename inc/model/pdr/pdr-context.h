#ifndef PDRCONTEXT_H
#define PDRCONTEXT_H

#include "cli-parse.h"
#include "pdr-model.h"
#include "tactic.h"

#include <cstdint>
#include <z3++.h>

namespace pdr
{
  // class containing algorithm settings
  // only consists of references or simple types
  // cheap to copy
  class Context
  {
   public:
    z3::context& z3_ctx;
    bool min_core;
    bool part_min_core;

    uint32_t seed;
    Tactic type;

    // After a cube is blocked, weaker or equal cubes may still be enqueued as
    // obligations.
    // If true (default): these are skipped to save on the number of
    // obligations considered.
    // If false: reconsider these weaker cubes to
    // potentially generalize them into a stronger cube
    bool skip_blocked;

    // in PDR::MIC if mic fails to reduce a clause this many times, consider the
    // current clause sufficient
    uint32_t mic_retries;
    // Frames refreshes its solver, removing subsumed cubes, once this fraction
    // of asserted clauses are subsumed
    double subsumed_cutoff;

    // the depth of counterexamples-to-generalization that are considered
    uint32_t ctg_max_depth;
    // the maximum number of counterexamples-to-generalization that are
    // considered per cube (resets if the cube is joined with a ctg)
    uint32_t ctg_max_counters;

    Context(z3::context& c, my::cli::ArgumentList const& args);
    // override seed value
    Context(z3::context& c, my::cli::ArgumentList const& args, unsigned s);

    operator z3::context&();
    operator const z3::context&() const;

    z3::context& operator()();

    std::string settings_str() const;

   private:
    void init_settings(my::cli::ArgumentList const& args);
  }; // class PDRcontext
} // namespace pdr
#endif // PDRCONTEXT_H
