#include "peterson-experiments.h"
#include "io.h"
#include "pdr-context.h"
#include "pdr.h"
#include "peterson-result.h"
#include <tabulate/markdown_exporter.hpp>

namespace pdr::peterson::experiments
{
  namespace
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
  } // namespace

  void peterson_run(
      PetersonModel& model, pdr::Logger& log, const my::cli::ArgumentList& args)
  {
    using fmt::format;
    using std::cout;
    using std::endl;
    using std::vector;
    using tabulate::Table;
    using namespace my::io;

    const fs::path model_dir   = setup_model_path(args);
    const fs::path run_dir     = setup_path(model_dir / folder_name(args));
    const std::string filename = file_name(args);
    std::ofstream latex        = trunc_file(run_dir, filename, "tex");
    std::ofstream raw          = trunc_file(run_dir, "raw-" + filename, "md");

    vector<PetersonResult> repetitions;
    vector<PetersonResult> control_repetitions;

    Table sample_table;
    {
      sample_table.add_row(peterson::result::summary_header);
      sample_table.format()
          .font_align(tabulate::FontAlign::right)
          .hide_border_top()
          .hide_border_bottom();
    }
    Table control_table;
    {
      control_table.add_row(peterson::result::summary_header);
      control_table.format() = sample_table.format();
    }

    unsigned start = args.starting_value.value_or(0);
    unsigned N     = args.exp_sample.value();
    std::string tactic_str{ pdr::tactic::to_string(args.tactic) };
    vector<unsigned> seeds(N);
    {
      srand(time(0));
      std::generate(seeds.begin(), seeds.end(), rand);
    }

    auto run = [&, N](
                   vector<PetersonResult>& reps, Table& t, bool control) -> Run
    {
      assert(reps.empty());

      cout << (control ? "control run" : "ipdr run") << endl;

      for (unsigned i = 0; i < N; i++)
      {
        // new context with new random seed
        pdr::Context ctx(model, args.delta, seeds[i]);
        cout << format("{}: {}", i, seeds[i]) << endl;
        IPDR opt(ctx, model, args, log);

        PetersonResult result = control ? opt.control_run(args.tactic, start)
                                        : opt.run(args.tactic, start);

        if (!result.all_holds())
          cout << "counter found !" << endl;

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
      auto dump = [&exporter, &raw](const PetersonResult& r, size_t i)
      {
        raw << format("### Sample {}", i) << endl;
        tabulate::Table t{ r.raw_table() };
        format_base(t);
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
} // namespace pdr::peterson::experiments
