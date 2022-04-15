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

  void model_run(pebbling::Model& model, pdr::Logger& log,
      const my::cli::ArgumentList& args)
  {
    using std::optional;

    unsigned N = args.exp_sample.value();
    cout << format("running {}, {} samples", model.name, N) << endl;
    std::vector<Result> res;
    optional<unsigned> optimum;
    for (unsigned i = 0; i < N; i++)
    {
std::optional<unsigned> r;
      pdr::Context ctx(model, args.delta, true); // new context with new random seed
      pdr::pebbling::Optimizer opt(ctx, log);

      if (i == 0)
         optimum = opt.run(args);
      else
      {
        r = opt.run(args);
        assert(optimum == r);
      }
      
    }

    /* result format
      <averaged results>
      ------------------
      <complete results>
    */
  }

  ExperimentResult pdr_run(pdr::PDR& alg, Tactic tactic)
  {
    Results iteration_results(alg.get_ctx().model());
    const pebbling::Model& m = alg.get_ctx();
    cout << "sample run" << endl;
    switch (tactic)
    {
      case Tactic::increment:
      {
        unsigned N      = m.get_f_pebbles();
        pdr::Result res = alg.run(Tactic::basic, N);
        iteration_results << res;

        if (!res) // trace found
        {
        }
        else
        {
          for (N = N + 1; N <= m.n_nodes(); N++)
          {
            res = alg.increment_run(N);
            iteration_results << res;

            if (!res)
              break;
          }
        }
        assert(!res || N > m.n_nodes());
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
