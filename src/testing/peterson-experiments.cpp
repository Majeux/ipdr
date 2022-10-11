#include "peterson-experiments.h"
#include "io.h"
#include "pdr.h"
#include "peterson-result.h"
#include "pdr-context.h"

namespace pdr::peterson::experiments
{

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

    unsigned N = args.exp_sample.value();
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
        std::optional<unsigned> optimum;
        // new context with new random seed
        pdr::Context ctx(model, args.delta, seeds[i]);
        cout << format("{}: {}", i, seeds[i]) << endl;
        pdr::pebbling::IPDR opt(ctx, model, args, log);

        PetersonResult result =
            control ? opt.control_run(args.tactic) : opt.run(args.tactic);

        if (!optimum)
          optimum = result.min_pebbles();

        assert(optimum == result.min_pebbles()); // all results should be same

        reps.push_back(result);
        // cout << format("## Experiment sample {}", i) << endl;
        reps.back().add_to_table(t);
      }

      return Run(args, reps);
    };
  }
} // namespace pdr::peterson::experiments
