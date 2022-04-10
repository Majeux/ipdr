#include "experiments.h"
#include "pdr-context.h"
#include "pdr.h"
#include <cassert>
#include <fmt/core.h>
#include <variant>

namespace pdr::experiments
{
  using fmt::format;
  using std::cout;
  using std::endl;

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
    cout << format("running {}, {} samples", model.name, sample_size) << endl;
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

  void pdr_run(pdr::PDR& alg, Tactic tactic)
  {
    Results iteration_results(alg.get_ctx().model());
    const PebblingModel& m = alg.get_ctx().model();
    cout << "sample run" << endl;
    switch (tactic)
    {
      case Tactic::increment:
      {
        unsigned N            = m.get_f_pebbles();
        pdr::Result invariant = alg.run(Tactic::basic, N);
        iteration_results << invariant;

        if (!invariant)
        {
          
        }

        for (N = N+1; N <= m.n_nodes(); N++)
        {
          invariant = alg.increment_run(N);
          iteration_results << invariant;
          
        }
        while (invariant)
        {
          N++;
          if (N > m.n_nodes())
            break;
          invariant = alg.increment_run(N);
          iteration_results << invariant;

        }
      }
      break;
      case Tactic::decrement:
      {
        unsigned N            = m.n_nodes();
        pdr::Result invariant = alg.run(Tactic::basic, N);
        iteration_results << invariant;
        while (invariant)
        {
          N--;
          if (N <= m.get_f_pebbles())
            break;
          invariant = alg.increment_run(N);
          iteration_results << invariant;
        }
      }
      break;
      default: assert(false);
    }
  }

} // namespace pdr::experiments
