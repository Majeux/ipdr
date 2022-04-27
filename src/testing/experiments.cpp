#include "experiments.h"
#include "pdr-context.h"
#include "pdr.h"
#include "tactic.h"
#include <algorithm>
#include <cassert>
#include <fmt/core.h>
#include <numeric>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/table.hpp>
#include <variant>

namespace pdr::experiments
{
  using fmt::format;
  using std::cout;
  using std::endl;

  Run::Run(const my::cli::ArgumentList& args,
      const std::vector<ExperimentResults>& results)
      : model(args.model_name), tactic(args.tactic), avg_time(0.0)
  {
    using std::min;
    using Invariant = Result::Invariant;
    using Trace     = Result::Trace;

    double time_sum = 0.0;
    std::vector<double> times;
    for (const ExperimentResults& r : results)
    {
      auto [t, inv, trace] = r.get_total();

      time_sum += t;
      times.push_back(t);

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
    }
    avg_time = time_sum / results.size();

    double total_variance = std::accumulate(times.begin(), times.end(), 0.0,
        [this](double a, double t) { return a + std::pow(t - avg_time, 2); });

    std_dev_time = std::sqrt(total_variance / results.size());
  }

  namespace
  {
    tabulate::Table init_table()
    {
      tabulate::Table t;
      t.format()
          .font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
      return t;
    }
  } // namespace

  std::string Run::str() const
  {
    tabulate::Table table = init_table();

    for (const Row_t& r : listing())
      table.add_row(r);

    std::stringstream ss;
    ss << fmt::format("Experiment: {}", model) << endl << table;

    return ss.str();
  }

  std::string Run::str_compared(const Run& other) const
  {
    tabulate::Table paired = init_table();

    for (const Row_t& r : combined_listing(other))
      paired.add_row(r);

    std::stringstream ss;
    ss << fmt::format("Experiment: {}", model) << endl << paired;

    return ss.str();
  }

  Run::Table_t Run::listing() const
  {

    Table_t t;
    {
      using fmt::to_string;
      size_t i = 0;

      t.at(i++) = { "", tactic::to_string(tactic) };
      t.at(i++) = { "avg time", to_string(avg_time) };
      t.at(i++) = { "std dev time", to_string(std_dev_time) };

      if (max_inv)
      {
        t.at(i++) = { "max_inv constraint",
          to_string(max_inv->constraint.value()) };
        t.at(i++) = { "max_inv level", to_string(max_inv->level) };
      }

      if (min_strat)
      {
        t.at(i++) = { "min strat marked", to_string(min_strat->marked) };
        t.at(i++) = { "min strat length", to_string(min_strat->length) };
      }
    }
    return t;
  }

  namespace
  {
    template <typename T> double percentage_dec(T old_v, T new_v)
    {
      double a = old_v;
      double b = new_v;
      return (double)(a - b) / a * 100;
    }

    template <typename T> double percentage_inc(T old_v, T new_v)
    {
      double a = old_v;
      double b = new_v;
      return (b - a) / a * 100;
    }
  } // namespace

  Run::Table_t Run::combined_listing(const Run& other) const
  {
    std::string percentage_fmt{"{:.2f} %"};

    Table_t rows = listing();
    {
      using fmt::to_string;
      size_t i = 0;
      {
        rows.at(i).push_back("control");
        rows.at(i).push_back("improvement");
        i++;
      }
      
      {
        rows.at(i).push_back(to_string(other.avg_time));
        // double speedup = (other.avg_time - avg_time / other.avg_time) * 100;
        double speedup = percentage_dec(other.avg_time, avg_time);
        rows.at(i).push_back(format(percentage_fmt, speedup));
        i++;
      }

      rows.at(i++).push_back(to_string(other.std_dev_time));
      
      if (other.max_inv)
      {
        rows.at(i++).push_back(to_string(other.max_inv->constraint.value()));
        {
          rows.at(i).push_back(to_string(other.max_inv->level));
          double dec = percentage_dec(other.max_inv->level, max_inv->level);
          rows.at(i).push_back(format(percentage_fmt, dec));
          i++;
        }
      }
      
      if (other.min_strat)
      {
        rows.at(i++).push_back(to_string(other.min_strat->marked));
        {
          rows.at(i).push_back(to_string(other.min_strat->length));
          double dec =
              percentage_dec(other.min_strat->length, min_strat->length);
          rows.at(i).push_back(format(percentage_fmt, dec));
          i++;
        }
      }
    }

    return rows;
  }

  // FUNCTIONS
  //
  void model_run(pebbling::Model& model, pdr::Logger& log,
      const my::cli::ArgumentList& args)
  {
    using std::optional;

    optional<unsigned> optimum;
    std::vector<ExperimentResults> results;
    std::vector<ExperimentResults> control;

    tabulate::Table::Row_t header{ "runtime", "max constraint with invariant",
      "level", "min constraint with strategy", "length" };

    tabulate::Table sample_table;
    {
      sample_table.add_row(header);
      sample_table.format()
          .font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
    }
    tabulate::Table control_table;
    {
      control_table.add_row(header);
      control_table.format() = sample_table.format();
    }

    unsigned N = args.exp_sample.value();

    std::vector<unsigned> seeds(N);
    {
      srand(time(0));
      std::generate(seeds.begin(), seeds.end(), rand);
    }

    std::string tactic_str = pdr::tactic::to_string(args.tactic);
    cout << format("{} run. {} samples. {} tactic", model.name, N, tactic_str)
         << endl;
    // normal runs
    for (unsigned i = 0; i < N; i++)
    {
      std::optional<unsigned> r;
      // new context with new random seed
      pdr::Context ctx(model, args.delta, seeds[i]);
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

    cout << format("{} control run. {} samples. {} tactic", model.name, N,
                tactic_str)
         << endl;

    // control runs without incremental functionality
    for (unsigned i = 0; i < N; i++)
    {
      std::optional<unsigned> r;
      // new context with new random seed
      pdr::Context ctx(model, args.delta, seeds[i]);
      pdr::pebbling::Optimizer opt(ctx, log);

      if (i == 0)
        optimum = opt.control_run(args);
      else
      {
        r = opt.control_run(args);
        assert(optimum == r);
      }

      control.emplace_back(opt.latest_results, args.tactic);
      // cout << format("## Experiment sample {}", i) << endl;
      control.back().add_to(control_table);
    }

    Run aggregate(args, results);
    Run control_aggregate(args, control);
    cout << aggregate.str_compared(control_aggregate) << endl;

    cout << sample_table << endl;
    cout << control_table << endl;

    assert(results.size() == N);
    // results.at(0).show_raw(std::cout);

    // for (size_t i = 0; i < results.size(); i++)
    // {
    //   cout << std::endl << format("## Raw data sample {}", i) << endl;
    //   results[i].show_raw(std::cout);
    //   cout << endl << endl;
    // }

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
