#include "pebbling-experiments.h"
#include "pebbling-result.h"
#include "z3-pebbling-experiments.h"
#include "z3-pebbling-model.h"
#include "z3pdr.h"

#include <fmt/format.h>
#include <memory>
#include <vector>

namespace pdr::test::experiments
{
  using std::shared_ptr;
  using std::unique_ptr;
  using std::vector;

  Z3PebblingExperiment::Z3PebblingExperiment(
      my::cli::ArgumentList const& a, Z3PebblingModel& m, Logger& l)
      : expsuper::Experiment(a, l), ts(m)
  {
  }

  void Z3PebblingExperiment::reset_tables()
  {
    sample_table          = tabulate::Table();
    sample_table.format() = control_table.format();
    sample_table.add_row(pebbling::IpdrPebblingResult::pebbling_total_header);

    control_table          = tabulate::Table();
    control_table.format() = sample_table.format();
    control_table.add_row(pebbling::IpdrPebblingResult::pebbling_total_header);
  }

  shared_ptr<expsuper::Run> Z3PebblingExperiment::do_reps(const bool is_control)
  {
    using pebbling::IpdrPebblingResult;
    using pebbling::experiments::PebblingRun;

    assert(is_control);
    vector<unique_ptr<IpdrResult>> results;

    for (unsigned i = 0; i < N_reps; i++)
    {
      std::cout << fmt::format("{}: {}", i, seeds[i]) << std::endl;
      std::optional<unsigned> optimum;
      // new context with new random seed
      pdr::Context ctx(ts.ctx, seeds[i]);
      z3PebblingIPDR opt(args, ctx, log, ts);
      {
        IpdrPebblingResult result = opt.control_run(tactic);

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

} // namespace pdr::test::experiments
