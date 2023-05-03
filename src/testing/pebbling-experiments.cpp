#include "pebbling-experiments.h"
#include "cli-parse.h"
#include "experiments.h"
#include "io.h"
#include "math.h"
#include "pdr-context.h"
#include "pdr.h"
#include "pebbling-result.h"
#include "result.h"
#include "stats.h"
#include "tactic.h"
#include "types-ext.h"

#include "cli-parse.h"
#include <cassert>
#include <fmt/format.h>
#include <iterator>
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
      my::cli::ArgumentList const& a, Logger& l)
      : expsuper::Experiment(a, l)
  {
    using my::variant::get_cref;

    ts_descr = get_cref<model_t::Pebbling>(args.model).value();
  }

  void PebblingExperiment::reset_tables()
  {
    sample_table          = tabulate::Table();
    sample_table.format() = control_table.format();
    sample_table.add_row(IpdrPebblingResult::pebbling_total_header);

    control_table          = tabulate::Table();
    control_table.format() = sample_table.format();
    control_table.add_row(IpdrPebblingResult::pebbling_total_header);
  }

  shared_ptr<expsuper::Run> PebblingExperiment::do_reps(const bool is_control)
  {
    vector<unique_ptr<IpdrResult>> results;

    for (unsigned i = 0; i < N_reps; i++)
    {
      cout << format("{}: {}", i, seeds[i]) << endl;
      std::optional<unsigned> optimum;
      // new context with new random seed
      z3::context z3_ctx;
      pdr::Context ctx(z3_ctx, args, seeds[i]);

      dag::Graph G = model_t::make_graph(ts_descr.src);
      PebblingModel ts(args, z3_ctx, G);

      IPDR opt(args, ctx, log, ts);
      {
        IpdrPebblingResult result =
            is_control ? opt.control_run(tactic) : opt.run(tactic);

        if (!optimum)
          optimum = result.min_pebbles();

        assert(optimum == result.min_pebbles()); // all results should be same

        results.emplace_back(
            std::make_unique<IpdrPebblingResult>(std::move(result)));
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
        auto const& pebbling_r = dynamic_cast<IpdrPebblingResult const&>(*r);
        IpdrPebblingResult::Data_t total = pebbling_r.get_total();

        // time is done by Run()

        // from all the totals:
        if (total.inv) // get the lowest invariant level we found
        {
          if (!min_inv || total.inv->invariant.level < min_inv->invariant.level)
            min_inv = total.inv;
        }

        if (total.strategy) // get the shortest trace we found
        {
          if (!min_strat || total.strategy->length < min_strat->length)
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
    return { "min strat marked", fmt::to_string(min_strat->n_marked) };
  }

  tabulate::Table::Row_t PebblingRun::length_row() const
  {
    return { "min strat length", fmt::to_string(min_strat->length) };
  }

  namespace
  {
    size_t n_rows(tabulate::Table& t)
    {
      size_t rows{ 0 };
      auto it = t.begin();
      while (it != t.end())
      {
        rows++;
        ++it;
      }

      return rows;
    }
  } // namespace

  tabulate::Table PebblingRun::make_table() const
  {
    tabulate::Table t = Run::make_table();
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

    return t;
  }

  tabulate::Table PebblingRun::make_combined_table(const Run& control) const
  {
    using namespace my::math;
    using fmt::to_string;

    auto perc_str = [](double x) { return format("{:.2f} \\\%", x); };

#warning !! takes the minimal trace, should take minimal trace of latest run (with smallest pebbles)
    tabulate::Table t = Run::make_combined_table(control);
    try
    {
      auto const& pebbling_ctrl = dynamic_cast<PebblingRun const&>(control);
      // append control and improvement column:
      // tactic | control | improvement (%)
      if (pebbling_ctrl.min_inv)
      {
        {
          auto r = constraint_row();
          r.push_back(
              to_string(pebbling_ctrl.min_inv->constraint.value()));
          t.add_row(r);
        }
        {
          auto r = level_row();
          r.push_back(
              to_string(pebbling_ctrl.min_inv->constraint.value()));
          double dec = percentage_dec(pebbling_ctrl.min_inv->constraint.value(),
              min_inv->constraint.value());
          r.push_back(perc_str(dec));
          t.add_row(r);
        }
      }
      if (pebbling_ctrl.min_inv)
      {
        {
          auto r = pebbled_row();
          r.push_back(fmt::to_string(pebbling_ctrl.min_strat->n_marked));
          t.add_row(r);
        }
        {
          auto r = length_row();
          r.push_back(fmt::to_string(pebbling_ctrl.min_strat->length));
          double dec = percentage_dec(
              pebbling_ctrl.min_strat->length, min_strat->length);
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

    return t;
  }
} // namespace pdr::pebbling::experiments
