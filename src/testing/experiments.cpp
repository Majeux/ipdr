#include "experiments.h"
#include "pdr-context.h"
#include "pdr.h"
#include "tactic.h"
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
    cout << format("running {}. {} samples. {} tactic", model.name, N,
                pdr::tactic::to_string(args.tactic))
         << endl;
    optional<unsigned> optimum;
    for (unsigned i = 0; i < N; i++)
    {
      std::optional<unsigned> r;
      pdr::Context ctx(
          model, args.delta, true); // new context with new random seed
      pdr::pebbling::Optimizer opt(ctx, log);

      if (i == 0)
        optimum = opt.run(args);
      else
      {
        r = opt.run(args);
        assert(optimum == r);
      }

      ExperimentResults er(opt.latest_results, args.tactic);
      cout << format("## Experiment sample {}", i) << endl;
      er.show(cout);
      cout << std::endl << format("## Raw data sample {}", i) << endl;
      er.show_raw(std::cout);
      cout << endl
           << endl
           << "==================================" << endl
           << endl;
    }

    /* result format
      <averaged results>
      ------------------
      <complete results>
    */
  }

  // double ExperimentResults::median_time()
  // {
  //   std::vector<double> times = extract_times();
  //   std::sort(times.begin(), times.end());

  //   if (times.size() % 2 == 0) // even number
  //   {
  //     size_t i1 = times.size() / 2;
  //     size_t i2 = i1 + 1;
  //     return (times[i1] + times[i2]) / 2; // return average of middle two
  //   }
  //   else
  //     return times[times.size() / 2];
  // }

  // double ExperimentResults::mean_time()
  // {
  //   double total = std::accumulate(original.begin(), original.end(), 0.0,
  //       [](double a, const Result& r) { return a + r.time; });
  //   return total / original.size();
  // }

  // double ExperimentResults::time_std_dev(double mean)
  // {
  //   std::vector<double> times = extract_times();
  //   double diffs              = std::accumulate(times.begin(), times.end(),
  //   0.0,
  //                    [mean](double a, double t) { return a + std::sqrt(t -
  //                    mean); });
  //   double variance           = diffs / times.size();

  //   return std::sqrt(variance);
  // }

} // namespace pdr::experiments
