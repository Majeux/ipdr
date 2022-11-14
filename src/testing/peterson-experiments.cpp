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

#include <cassert>
#include <fmt/format.h>
#include <memory>
#include <numeric> // std::accumulate
#include <ostream> //std::ofstream
#include <sstream>
#include <string>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include <vector>

namespace pdr::peterson::experiments
{
  using fmt::format;
  using std::cout;
  using std::endl;
  using std::shared_ptr;
  using std::unique_ptr;
  using std::vector;

  PeterExperiment::PeterExperiment(
      my::cli::ArgumentList const& a, PetersonModel& m, Logger& l)
      : Experiment(a, l), ts(m)
  {
    auto peter = my::variant::get_cref<my::cli::model_t::Peterson>(args.model);
    ts_descr   = peter->get();
  }

  unique_ptr<PeterExperiment::superRun> PeterExperiment::single_run(
      bool is_control)
  {
    vector<PetersonResult> results;

    for (unsigned i = 0; i < N_reps; i++)
    {
      cout << format("{}: {}", i, seeds[i]) << endl;
      // new context with new random seed
      pdr::Context ctx(ts, seeds[i]);
      IPDR opt(ctx, ts, args, log);

      PetersonResult result = is_control
                                ? opt.control_run(tactic, ts_descr.start)
                                : opt.run(tactic, ts_descr.start);

      if (!result.all_holds())
        cout << "! counter found" << endl;

      results.push_back(std::move(result));
      results.back().add_summary_to(is_control ? control_table : sample_table);
    }

    return std::make_unique<PeterRun>(args, results);
  }

  // Run members
  //
  // aggregate multiple experiments and format
  PeterRun::PeterRun(const my::cli::ArgumentList& args,
      const std::vector<PetersonResult>& results)
      : Run({results.cbegin(), results.cend()}),  correct(true)
  {
  }

  PeterRun::Table_t PeterRun::listing() const
  {
    Table_t t;
    {
      using fmt::to_string;
      size_t i = 0;

      t.at(i++) = { "", tactic::to_string(tactic) };
      t.at(i++) = { "avg time", math::time_str(avg_time) };
      t.at(i++) = { "std dev time", math::time_str(std_dev_time) };

      t.at(i++) = { "all hold", correct ? "yes" : "no" };
      assert(i == t.size());
    }
    return t;
  }

  PeterRun::Table_t PeterRun::combined_listing(const Run& control) const
  {
    std::string percentage_fmt{ "{:.2f} \\\%" };
    auto perc_str = [](double x) { return format("{:.2f} \\\%", x); };

    Table_t rows = listing();
    {
      using fmt::to_string;
      size_t i = 0;
      // append control column
      {
        rows.at(i).push_back("control");
        rows.at(i).push_back("improvement");
        i++;
      }

      {
        rows.at(i).push_back(math::time_str(control.avg_time));
        // double speedup = (other.avg_time - avg_time / other.avg_time) * 100;
        double speedup = math::percentage_dec(control.avg_time, avg_time);
        rows.at(i).push_back(perc_str(speedup));
        i++;
      }

      rows.at(i++).push_back(math::time_str(control.std_dev_time));
      rows.at(i++).push_back(control.correct ? "yes" : "no");
      assert(i == rows.size());
    }

    return rows;
  }
} // namespace pdr::peterson::experiments
