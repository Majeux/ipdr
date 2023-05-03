#include "experiments.h"
#include "cli-parse.h"
#include "math.h"
#include "pebbling-result.h"
#include "result.h"
#include "stats.h"
#include "tactic.h"
#include "types-ext.h"

#include <cassert>
#include <fmt/core.h>
#include <fmt/format.h>
#include <tabulate/exporter.hpp>
#include <tabulate/format.hpp>
#include <tabulate/latex_exporter.hpp>
#include <tabulate/markdown_exporter.hpp>
#include <tabulate/table.hpp>
#include <variant>

namespace pdr::experiments
{
  using namespace my::cli;
  using namespace my::math;
  using fmt::format;
  using std::vector;

  std::string time_str(double x) { return fmt::format("{:.3f} s", x); };

  namespace tablef
  {
    tabulate::Format& format_base(tabulate::Format& f)
    {
      return f.font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
    }

    tabulate::Format& format_base(tabulate::Table& t)
    {
      return format_base(t.format());
    }

    tabulate::Table init_table()
    {
      tabulate::Table t;
      format_base(t);
      return t;
    }
  } // namespace tablef

  // RUN PUBLIC MEMBERS
  //
  Run::Run(std::string const& m, std::string const& t,
      std::vector<std::unique_ptr<IpdrResult>>&& r)
      : results(std::move(r)), model(m), tactic(t)
  {
    double time_sum{ 0.0 }, inc_time_sum{ 0.0 };
    vector<double> times, inc_times;
    for (auto const& r : results)
    {
      {
        double dt = r->get_total_time();
        time_sum += dt;
        times.push_back(dt);
      }
      if (auto dt_inc = r->get_inc_time())
      {
        inc_time_sum += *dt_inc;
        inc_times.push_back(*dt_inc);
      }
    }

    assert(times.size() == results.size());
    avg_time     = time_sum / times.size();
    std_dev_time = std_dev(times, avg_time);

    if (!inc_times.empty())
    {
      assert(inc_times.size() == results.size());
      avg_inc_time     = inc_time_sum / inc_times.size();
      std_dev_inc_time = std_dev(inc_times, avg_inc_time.value());
    }
  }

  tabulate::Table::Row_t Run::tactic_row() const { return { "", tactic }; }

  tabulate::Table::Row_t Run::avg_time_row() const
  {
    return { "avg time", time_str(avg_time) };
  }

  tabulate::Table::Row_t Run::std_time_row() const
  {
    return { "std dev time", time_str(std_dev_time) };
  }

  tabulate::Table::Row_t Run::avg_inc_time_row() const
  {
    return { "avg incremental time",
      avg_inc_time ? time_str(*avg_inc_time) : "-" };
  }

  tabulate::Table::Row_t Run::std_inc_time_row() const
  {
    return { "std dev incremental time",
      std_dev_inc_time ? time_str(*std_dev_inc_time) : "-" };
  }

  tabulate::Table Run::make_table() const
  {
    tabulate::Table t;
    t.add_row(tactic_row());
    t.add_row(avg_time_row());
    t.add_row(std_time_row());
    if (avg_inc_time)
    {
      assert(std_dev_inc_time);
      t.add_row(avg_inc_time_row()); // control should have no inc_time
      t.add_row(std_inc_time_row());
    }
    else {
      t.add_row({});
      t.add_row({});
    }

    return t;
  }

  tabulate::Table Run::make_combined_table(const Run& control) const
  {
    using namespace my::math;

    std::string percentage_fmt{ "{:.2f} \\\%" };
    auto perc_str = [](double x) { return format("{:.2f} \\\%", x); };

    tabulate::Table t;
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
      r.push_back(time_str(control.avg_time));
      double speedup = percentage_dec(control.avg_time, avg_time);
      r.push_back(perc_str(speedup));
      t.add_row(r);
    }
    {
      auto r = std_time_row();
      r.push_back(time_str(control.std_dev_time));
      t.add_row(r);
    }
    if (avg_inc_time)
    {
      assert(std_dev_inc_time);
      t.add_row(avg_inc_time_row()); // control should have no inc_time
      t.add_row(std_inc_time_row());
    }
    else {
      t.add_row({});
      t.add_row({});
    }

    return t;
  }

  std::string Run::str(output_format form) const
  {
    using std::endl;
    // tabulate::Table table = tablef::init_table();
    tabulate::Table table = make_table();
    tablef::format_base(table);

    std::stringstream ss;
    switch (form)
    {
      case output_format::string:
        ss << fmt::format("Experiment: {}", model) << endl << table;
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

  std::string Run::str_compared(const Run& other, output_format form) const
  {
    using std::endl;
    tabulate::Table paired = make_combined_table(other);
    tablef::format_base(paired);

    std::stringstream ss;
    switch (form)
    {
      case output_format::string:
        ss << fmt::format("Experiment: {}", model) << endl << paired;
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

  void Run::dump(tabulate::Exporter& exporter, std::ostream& out) const
  {
    using std::endl;

    for (size_t i{ 0 }; i < results.size(); i++)
    {
      out << fmt::format("### Sample {}", i) << endl;
      tabulate::Table t{ results[i]->summary_table() };
      tablef::format_base(t);
      out << exporter.dump(t) << endl << endl;

      out << "#### Traces" << endl;
      out << results[i]->all_traces() << endl;
    }
  }

  // EXPERIMENT PUBLIC MEMBERS
  //
  Experiment::Experiment(my::cli::ArgumentList const& a, Logger& l)
      : args(a),
        model(model_t::get_name(args.model)),
        type(algo::get_name(args.algorithm)),
        log(l),
        N_reps(args.experiment->repetitions),
        seeds(N_reps)
  {
    if (auto ipdr = my::variant::get_cref<algo::t_IPDR>(args.algorithm))
      tactic = ipdr->get().type;

    sample_table.format()
        .font_align(tabulate::FontAlign::right)
        .hide_border_top()
        .hide_border_bottom();
    control_table.format() = sample_table.format();

    srand(time(0));
    if (args.experiment->seeds)
      seeds = args.experiment->seeds.value();
    else
      std::generate(seeds.begin(), seeds.end(), rand);
  }

  void Experiment::run()
  {
    using fmt::format;
    using std::endl;

    log.graph.reset(model_t::get_name(args.model));
    reset_tables();

    std::ofstream latex = args.folders.file_in_run("tex");
    std::ofstream raw   = args.folders.file_in_run("md");
    std::ofstream graph = args.folders.file_in_analysis("tex");
    tabulate::MarkdownExporter exporter;

    if (args.control_run)
    {
      std::cout << type + " (only) control run." << endl;
      std::shared_ptr<Run> control_aggregate = do_reps(true);
      assert(control_aggregate != nullptr);
      latex << control_aggregate->str(output_format::latex);

      // write raw run data as markdown
      raw << format("# {}. {} samples. {} tactic.", model, N_reps,
                 tactic::to_string(tactic))
          << endl
          << format("seeds used: {}", fmt::join(seeds, ", ")) << endl;

      raw << "## Control run." << endl;
      control_aggregate->dump(exporter, raw);
    }
    else
    {
      std::cout << type + " run." << endl;

      log.graph.reset(model_t::src_name(args.model));
      std::shared_ptr<Run> aggregate = do_reps(false);
      latex << aggregate->str(output_format::latex);

      Graphs run_graph = log.graph;
      log.graph.reset(model_t::get_name(args.model));

      std::cout << "control run." << endl;
      std::shared_ptr<Run> control_aggregate = do_reps(true);
      assert(control_aggregate != nullptr);
      latex << aggregate->str_compared(
          *control_aggregate, output_format::latex);

      // write raw run data as markdown
      raw << format("# {}. {} samples. {} tactic.", model, N_reps,
                 tactic::to_string(tactic))
          << endl
          << format("seeds used: {}", fmt::join(seeds, ", ")) << endl;

      raw << "## Experiment run." << endl;
      aggregate->dump(exporter, raw);

      raw << "## Control run." << endl;
      control_aggregate->dump(exporter, raw);

      graph << Graphs::combine(run_graph, log.graph) << endl;
      // graph << log.graph.get_cti() << endl
      //           << log.graph.get_obligation() << endl
      //           << log.graph.get_sat() << endl
      //           << log.graph.get_relax() << endl;
    }
  }
} // namespace pdr::experiments
