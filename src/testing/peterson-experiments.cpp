#include "peterson-experiments.h"
#include "cli-parse.h"
#include "experiments.h"
#include "io.h"
#include "pdr-context.h"
#include "pdr.h"
#include "peterson-result.h"
#include "peterson.h"
#include "result.h"
#include "types-ext.h"
#include "math.h"

#include <cassert>
#include <fmt/format.h>
#include <memory>
#include <ostream> //std::ofstream
#include <sstream>
#include <stdexcept>
#include <string>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include <typeinfo>
#include <vector>

namespace pdr::peterson::experiments
{
  using namespace my::cli;
  using fmt::format;
  using std::cout;
  using std::endl;
  using std::shared_ptr;
  using std::unique_ptr;
  using std::vector;

  PetersonExperiment::PetersonExperiment(
      my::cli::ArgumentList const& a, Logger& l)
      : Experiment(a, l)
  {
    using my::variant::get_cref;

    ts_descr = get_cref<model_t::Peterson>(args.model)->get();
  }

  void PetersonExperiment::reset_tables()
  {
    sample_table          = tabulate::Table();
    sample_table.format() = control_table.format();
    sample_table.add_row(IpdrPetersonResult::peterson_total_header);

    control_table          = tabulate::Table();
    control_table.format() = sample_table.format();
    control_table.add_row(IpdrPetersonResult::peterson_total_header);
  }

  shared_ptr<expsuper::Run> PetersonExperiment::do_reps(const bool is_control)
  {
    vector<unique_ptr<IpdrResult>> results;

    for (unsigned i = 0; i < N_reps; i++)
    {
      cout << format("{}: {}", i, seeds[i]) << endl;
      // new context with new random seed
      z3::context z3_ctx;
      pdr::Context ctx(z3_ctx, args);
      ctx.seed = seeds[i];

      PetersonModel ts(z3_ctx, ts_descr.start, ts_descr.max);

      IPDR opt(args, ctx, log, ts);
      {
        IpdrPetersonResult result = is_control
                                      ? opt.control_run(tactic, ts_descr.start)
                                      : opt.run(tactic, ts_descr.start);

        if (!result.all_holds())
          cout << "! counter found" << endl;

        results.emplace_back(
            std::make_unique<IpdrPetersonResult>(std::move(result)));
      }

      if (is_control)
        control_table.add_row(results.back()->total_row());
      else
        sample_table.add_row(results.back()->total_row());
    }

    return std::make_shared<PeterRun>(model, type, std::move(results));
  }

  // Run members
  //
  // aggregate multiple experiments and format
  PeterRun::PeterRun(std::string const& m, std::string const& t,
      std::vector<std::unique_ptr<IpdrResult>>&& r)
      : Run(m, t, std::move(r)), correct(true)
  {
    for (auto const& r : results)
    {
      try
      {
        auto const& peter_r = dynamic_cast<IpdrPetersonResult const&>(*r);

        // time is done by Run()
        correct = correct && peter_r.all_holds();
      }
      catch (std::bad_cast const& e)
      {
        throw std::invalid_argument(
            "PeterRun expects a vector of PetersonResult ptrs");
      }
    }
  }

  tabulate::Table::Row_t PeterRun::correct_row() const
  {
    return { "all hold", correct ? "yes" : "no" };
  }

  tabulate::Table PeterRun::make_table() const
  {
    tabulate::Table t;
    {
      using fmt::to_string;

      t.add_row(tactic_row());
      t.add_row(avg_time_row());
      t.add_row(std_time_row());

      t.add_row(correct_row());
    }

    assert(t.shape().first == 4); // n_rows == 4

    return t;
  }

  tabulate::Table PeterRun::make_combined_table(const Run& control) const
  {
    using namespace my::math;
    using pdr::experiments::time_str;
    using fmt::to_string;

    std::string percentage_fmt{ "{:.2f} \\\%" };
    auto perc_str = [](double x) { return format("{:.2f} \\\%", x); };

    tabulate::Table t;
    try
    {
      auto const& peter_control = dynamic_cast<PeterRun const&>(control);
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
        r.push_back(time_str(peter_control.avg_time));
        double speedup = percentage_dec(peter_control.avg_time, avg_time);
        r.push_back(perc_str(speedup));
        t.add_row(r);
      }
      {
        auto r = std_time_row();
        r.push_back(time_str(peter_control.std_dev_time));
        t.add_row(r);
      }
      {
        auto r = correct_row();
        r.push_back(peter_control.correct ? "yes" : "no");
        t.add_row(r);
      }
    }
    catch (std::bad_cast const& e)
    {
      throw std::invalid_argument("combined_listing expects a PeterRun const&");
    }

    assert(t.shape().first == 4); // n_rows == 4

    return t;
  }
} // namespace pdr::peterson::experiments
