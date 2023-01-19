#include "pebbling-experiments.h"
#include "experiments.h"
#include "io.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-result.h"
#include "result.h"
#include "tactic.h"

#include "cli-parse.h"
#include <cassert>
#include <fmt/format.h>
#include <iterator>
#include <numeric> // std::accumulate
#include <ostream> //std::ofstream
#include <sstream>
#include <string>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>

#include <vector>

namespace pdr::pebbling::experiments
{
  using fmt::format;
  using std::cout;
  using std::endl;
  using std::shared_ptr;
  using std::unique_ptr;
  using std::vector;

  using namespace ::pdr::experiments;
  using namespace my::cli;

  PebblingExperiment::PebblingExperiment(
      my::cli::ArgumentList const& a, PebblingModel& m, Logger& l)
      : expsuper::Experiment(a, l), ts(m)
  {
  }

  void PebblingExperiment::reset_tables()
  {
    sample_table          = tabulate::Table();
    sample_table.format() = control_table.format();
    sample_table.add_row(PebblingResult::pebbling_total_header);

    control_table          = tabulate::Table();
    control_table.format() = sample_table.format();
    control_table.add_row(PebblingResult::pebbling_total_header);
  }

  shared_ptr<expsuper::Run> PebblingExperiment::do_reps(bool is_control)
  {
    vector<unique_ptr<IpdrResult>> results;

    for (unsigned i = 0; i < N_reps; i++)
    {
      cout << format("{}: {}", i, seeds[i]) << endl;
      std::optional<unsigned> optimum;
      // new context with new random seed
      pdr::Context ctx(ts.ctx, seeds[i]);
      IPDR opt(ctx, ts, args, log);

      {
        PebblingResult result =
            is_control ? opt.control_run(tactic) : opt.run(tactic);

        if (!optimum)
          optimum = result.min_pebbles();

        assert(optimum == result.min_pebbles()); // all results should be same

        results.emplace_back(
            std::make_unique<PebblingResult>(std::move(result)));
      }

      if (is_control)
        control_table.add_row(results.back()->total_row());
      else
        sample_table.add_row(results.back()->total_row());
    }

    return std::make_shared<PebblingRun>(model, type, std::move(results));
  }

  // Run members
  //
  // aggregate multiple experiments and format
  PebblingRun::PebblingRun(std::string const& m, std::string const& t,
      std::vector<std::unique_ptr<IpdrResult>>&& r)
      : Run(m, t, std::move(r))
  {
    for (auto const& r : results)
    {
      try
      {
        auto const& pebbling_r       = dynamic_cast<PebblingResult const&>(*r);
        PebblingResult::Data_t total = pebbling_r.get_total();

        // time is done by Run()

        // from all the totals:
        if (total.inv) // get the lowest invariant level we found
        {
          if (!min_inv || total.inv->invariant.level < min_inv->invariant.level)
            min_inv = total.inv;
        }

        if (total.strategy) // get the shortest trace we found
        {
          if (!min_strat ||
              total.strategy->trace.length < min_strat->trace.length)
            min_strat = total.strategy;
        }
      }
      catch (std::bad_cast const& e)
      {
        throw std::invalid_argument(
            "PebblingRun expects a vector of PebblingResult ptrs");
      }
    }
  }

  tabulate::Table::Row_t PebblingRun::constraint_row() const
  {
    return { "max inv constraint",
      fmt::to_string(min_inv->constraint.value()) };
  }

  tabulate::Table::Row_t PebblingRun::level_row() const
  {
    return { "min inv level", fmt::to_string(min_inv->invariant.level) };
  }

  tabulate::Table::Row_t PebblingRun::pebbled_row() const
  {
    return { "min strat marked", fmt::to_string(min_strat->pebbled) };
  }

  tabulate::Table::Row_t PebblingRun::length_row() const
  {
    return { "min strat length", fmt::to_string(min_strat->trace.length) };
  }

  tabulate::Table PebblingRun::make_table() const
  {
    tabulate::Table t;
    {
      t.add_row(tactic_row());
      t.add_row(avg_time_row());
      t.add_row(std_time_row());

      if (min_inv)
      {
        t.add_row(constraint_row());
        t.add_row(level_row());
      }
      else
      {
        t.add_row({});
        t.add_row({});
      }

      if (min_strat)
      {
        t.add_row(pebbled_row());
        t.add_row(length_row());
      }
      else
      {
        t.add_row({});
        t.add_row({});
      }
    }
    size_t rows{ 0 };
    auto it = t.begin();
    while (it != t.end())
    {
      rows++;
      ++it;
    }
    assert(rows == 7); // n_rows == 7
    return t;
  }

  tabulate::Table PebblingRun::make_combined_table(const Run& control) const
  {
    using namespace expsuper::math;
    using fmt::to_string;

    std::string percentage_fmt{ "{:.2f} \\\%" };
    auto perc_str = [](double x) { return format("{:.2f} \\\%", x); };

#warning !! takes the minimal trace, should take minimal trace of latest run (with smallest pebbles)
    tabulate::Table t;
    try
    {
      auto const& pebbling_ctrl = dynamic_cast<PebblingRun const&>(control);
      // append control and improvement column:
      // tactic | control | improvement (%)
      {
        auto r = tactic_row();
        r.push_back("control");
        r.push_back("improvement");
        t.add_row(r);
      }
      {
        auto r = avg_time_row();
        r.push_back(time_str(pebbling_ctrl.avg_time));
        double speedup = percentage_dec(pebbling_ctrl.avg_time, avg_time);
        r.push_back(perc_str(speedup));
        t.add_row(r);
      }
      {
        auto r = std_time_row();
        r.push_back(time_str(pebbling_ctrl.std_dev_time));
        t.add_row(r);
      }
      if (pebbling_ctrl.min_inv)
      {
        {
          auto r = constraint_row();
          r.push_back(
              fmt::to_string(pebbling_ctrl.min_inv->constraint.value()));
          t.add_row(r);
        }
        {
          auto r = level_row();
          r.push_back(
              fmt::to_string(pebbling_ctrl.min_inv->constraint.value()));
          double dec =
              math::percentage_dec(pebbling_ctrl.min_inv->constraint.value(),
                  min_inv->constraint.value());
          r.push_back(perc_str(dec));
          t.add_row(r);
        }
      }
      if (pebbling_ctrl.min_inv)
      {
        {
          auto r = pebbled_row();
          r.push_back(fmt::to_string(pebbling_ctrl.min_strat->pebbled));
          t.add_row(r);
        }
        {
          auto r = length_row();
          r.push_back(fmt::to_string(pebbling_ctrl.min_strat->trace.length));
          double dec = math::percentage_dec(
              pebbling_ctrl.min_strat->trace.length, min_strat->trace.length);
          r.push_back(perc_str(dec));
          t.add_row(r);
        }
      }
    }
    catch (std::bad_cast const& e)
    {
      throw std::invalid_argument(
          "combined_listing expects a PebblingRun const&");
    }
    assert(t.shape().first == 7); // n_rows == 4
    return t;
  }
} // namespace pdr::pebbling::experiments
