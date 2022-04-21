#include "experiments.h"
#include "pdr-context.h"
#include "pdr.h"
#include "tactic.h"
#include <cassert>
#include <fmt/core.h>
#include <tabulate/latex_exporter.hpp>
#include <variant>

namespace pdr::experiments
{
  using fmt::format;
  using std::cout;
  using std::endl;

  Run::Run(std::string_view m, const std::vector<ExperimentResults>& results)
      : model(m), avg_time(0.0)
  {
    using std::min;
    using Invariant = Result::Invariant;
    using Trace     = Result::Trace;

    double time_sum = 0.0;
    for (const ExperimentResults& r : results)
    {
      auto [t, inv, trace] = r.get_total();

      time_sum += t;

      if (inv) // get the lowest invariant level we found
      {
        if (max_inv)
          max_inv = min(*max_inv, *inv,
              [](Invariant a, Invariant b) { return a.level < b.level; });
        else
          max_inv = inv;
      }

      if (trace) // get the shortest trace we found
      {
        if (min_strat)
          min_strat = min(*min_strat, *trace,
              [](const Trace& a, const Trace& b)
              { return a.length < b.length; });
        else
          min_strat = trace;
      }

      avg_time = time_sum / results.size();
    }
  }

  void model_run(pebbling::Model& model, pdr::Logger& log,
      const my::cli::ArgumentList& args)
  {
    using std::optional;

    optional<unsigned> optimum;
    std::vector<ExperimentResults> results;
    tabulate::Table sample_table;
    sample_table.format()
        .font_align(tabulate::FontAlign::right)
        .hide_border_top()
        .hide_border_bottom();
    sample_table.add_row({ "runtime", "max constraint with invariant", "level",
        "min constraint with strategy", "length" });

    unsigned N = args.exp_sample.value();
    cout << format("running {}. {} samples. {} tactic", model.name, N,
                pdr::tactic::to_string(args.tactic))
         << endl;

    for (unsigned i = 0; i < N; i++)
    {
      std::optional<unsigned> r;
      // new context with new random seed
      pdr::Context ctx(model, args.delta, true);
      pdr::pebbling::Optimizer opt(ctx, log);

      if (i == 0)
        optimum = opt.run(args);
      else
      {
        r = opt.run(args);
        assert(optimum == r);
      }

      results.emplace_back(opt.latest_results, args.tactic);
      // cout << format("## Experiment sample {}", i) << endl;
      results.back().add_to(sample_table);
    }

    cout << sample_table << endl;

    assert(results.size() == N);
    for (size_t i = 0; i < results.size(); i++)
    {
      cout << std::endl << format("## Raw data sample {}", i) << endl;
      results[i].show_raw(std::cout);
      cout << endl << endl;
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
