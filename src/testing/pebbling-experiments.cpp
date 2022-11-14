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

  using namespace ::pdr::experiments;
  using namespace my::cli;

  void pebbling_run(
      PebblingModel& model, pdr::Logger& log, const my::cli::ArgumentList& args)
  {
    using std::optional;
    using std::vector;
    using tabulate::Table;

    using namespace my::io;

    assert(args.experiment);

    std::ofstream latex = args.folders.file_in_run("tex");
    std::ofstream raw   = args.folders.file_in_run("md");

    vector<PebblingResult> repetitions;
    vector<PebblingResult> control_repetitions;

    Table sample_table;
    {
      sample_table.add_row(pebbling::result::summary_header);
      sample_table.format()
          .font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
    }
    Table control_table;
    {
      control_table.add_row(pebbling::result::summary_header);
      control_table.format() = sample_table.format();
    }

    auto const& tactic = std::get<algo::t_IPDR>(args.algorithm).type;

    unsigned N = args.experiment->repetitions;
    std::string tactic_str{ pdr::tactic::to_string(tactic) };

    vector<unsigned> seeds(N);
    {
      srand(time(0));
      std::generate(seeds.begin(), seeds.end(), rand);
    }

    auto run = [&, N, tactic](
                   vector<PebblingResult>& reps, Table& t, bool control) -> Run
    {
      assert(reps.empty());

      cout << (control ? "control run" : "ipdr run") << endl;

      for (unsigned i = 0; i < N; i++)
      {
        std::optional<unsigned> optimum;
        // new context with new random seed
        pdr::Context ctx(model, seeds[i]);
        cout << format("{}: {}", i, seeds[i]) << endl;
        pdr::pebbling::IPDR opt(ctx, model, args, log);

        PebblingResult result =
            control ? opt.control_run(tactic) : opt.run(tactic);

        if (!optimum)
          optimum = result.min_pebbles();

        assert(optimum == result.min_pebbles()); // all results should be same

        reps.push_back(result);
        // cout << format("## Experiment sample {}", i) << endl;
        reps.back().add_summary_to(t);
      }

      return Run(args, reps);
    };

    cout << format("{} run. {} samples. {} tactic", model.name, N, tactic_str)
         << endl;

    Run aggregate = run(repetitions, sample_table, false);
    latex << aggregate.str(output_format::latex);

    Run control_aggregate = run(control_repetitions, control_table, true);
    latex << aggregate.str_compared(control_aggregate, output_format::latex);

    // write raw run data as markdown
    {
      tabulate::MarkdownExporter exporter;
      auto dump = [&exporter, &raw](const PebblingResult& r, size_t i)
      {
        raw << format("### Sample {}", i) << endl;
        tabulate::Table t{ r.raw_table() };
        tablef::format_base(t);
        raw << exporter.dump(t) << endl << endl;

        raw << "#### Traces" << endl;
        r.show_traces(raw);
      };

      raw << format("# {}. {} samples. {} tactic.", model.name, N, tactic_str)
          << endl;

      raw << "## Experiment run." << endl;
      for (size_t i = 0; i < repetitions.size(); i++)
        dump(repetitions[i], i);

      raw << "## Control run." << endl;
      for (size_t i = 0; i < repetitions.size(); i++)
        dump(control_repetitions[i], i);
    }
  }

  // Run members
  //
  // aggregate multiple experiments and format
  Run::Run(const my::cli::ArgumentList& args,
      const std::vector<PebblingResult>& results)
      : model(model_t::src_name(args.model)), tactic(args.tactic), avg_time(0.0)
  {
    using std::min;

    double time_sum{ 0.0 };
    std::vector<double> times;
    for (const PebblingResult& r : results)
    {
      PebblingResult::Data_t total = r.get_total();

      time_sum += total.time;
      times.push_back(total.time);

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
    assert(times.size() == results.size());
    avg_time = time_sum / times.size();

    std_dev_time = math::std_dev(times, avg_time);
  }

  std::string Run::str(output_format fmt) const
  {
    tabulate::Table table = tablef::init_table();

    for (const Row_t& r : table())
      table.add_row(r);

    std::stringstream ss;
    switch (fmt)
    {
      case output_format::string:
        ss << format("Experiment: {}", model) << endl << table;
        break;
      case output_format::latex:
        ss << tabulate::LatexExporter().dump(table) << endl;
        break;
      case output_format::markdown:
        ss << tabulate::MarkdownExporter().dump(table) << endl;
        break;
    }

    return ss.str();
  }

  std::string Run::str_compared(const Run& other, output_format fmt) const
  {
    tabulate::Table paired = tablef::init_table();

    for (const Row_t& r : combined_table(other))
      paired.add_row(r);

    std::stringstream ss;
    switch (fmt)
    {
      case output_format::string:
        ss << format("Experiment: {}", model) << endl << paired;
        break;
      case output_format::latex:
        ss << tabulate::LatexExporter().dump(paired) << endl;
        break;
      case output_format::markdown:
        ss << tabulate::MarkdownExporter().dump(paired) << endl;
        break;
    }
    return ss.str();
  }

  Run::Table_t Run::table() const
  {
    Table_t t;
    {
      using fmt::to_string;
      size_t i = 0;

      t.at(i++) = { "", tactic::to_string(tactic) };
      t.at(i++) = { "avg time", math::time_str(avg_time) };
      t.at(i++) = { "std dev time", math::time_str(std_dev_time) };

      if (min_inv)
      {
        t.at(i++) = { "max inv constraint",
          to_string(min_inv->constraint.value()) };
        t.at(i++) = { "min inv level", to_string(min_inv->invariant.level) };
      }

      if (min_strat)
      {
        t.at(i++) = { "min strat marked", to_string(min_strat->pebbled) };
        t.at(i++) = { "min strat length", to_string(min_strat->trace.length) };
      }
    }
    return t;
  }

  Run::Table_t Run::combined_table(const Run& other) const
  {
    std::string percentage_fmt{ "{:.2f} \\\%" };
    auto perc_str = [](double x) { return format("{:.2f} \\\%", x); };

    Table_t rows = table();
    {
      using fmt::to_string;
      size_t i = 0;
      {
        rows.at(i).push_back("control");
        rows.at(i).push_back("improvement");
        i++;
      }

      {
        rows.at(i).push_back(math::time_str(other.avg_time));
        // double speedup = (other.avg_time - avg_time / other.avg_time) * 100;
        double speedup = math::percentage_dec(other.avg_time, avg_time);
        rows.at(i).push_back(perc_str(speedup));
        i++;
      }

      rows.at(i++).push_back(math::time_str(other.std_dev_time));

      if (other.min_inv)
      {
        rows.at(i++).push_back(to_string(other.min_inv->constraint.value()));
        {
          rows.at(i).push_back(to_string(other.min_inv->invariant.level));
          double dec = math::percentage_dec(
              other.min_inv->invariant.level, min_inv->invariant.level);
          rows.at(i).push_back(perc_str(dec));
          i++;
        }
      }

      if (other.min_strat)
      {
        rows.at(i++).push_back(to_string(other.min_strat->pebbled));
        {
          rows.at(i).push_back(to_string(other.min_strat->trace.length));
          double dec = math::percentage_dec(
              other.min_strat->trace.length, min_strat->trace.length);
          rows.at(i).push_back(perc_str(dec));
          i++;
        }
      }
    }

    return rows;
  }

} // namespace pdr::pebbling::experiments
