#include "experiments.h"
#include "pdr-context.h"
#include "pdr.h"
#include <cassert>
#include <variant>

namespace pdr::experiments
{
  Run::Run(Tactic t, bool d, unsigned ss, std::optional<unsigned> p)
      : tactic(t), delta(d), sample_size(ss), n_pebbles(p)
  {
    switch (tactic)
    {
      case Tactic::decrement:
      case Tactic::increment: assert(!n_pebbles); break;
      default: assert(n_pebbles);
    }
  }

  void model_run(pdr::PebblingModel& model, pdr::Logger& log,
      unsigned sample_size, Tactic tactic, bool delta)
  {
    std::vector<Result> res;
    for (unsigned i = 0; i < sample_size; i++)
    {
      pdr::Context ctx(model, delta, true); // new context with new random seed
      pdr::PDR algorithm(ctx, log);
      pdr_run(algorithm, tactic);
    }

    /* result format
      <averaged results>
      ------------------
      <complete results>
    */
  }

  void pdr_run(pdr::PDR& alg, Tactic tactic) {}

} // namespace pdr::experiments
