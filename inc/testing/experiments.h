#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "logger.h"
#include "pdr-context.h"
#include "pdr.h"
#include "result.h"
#include <variant>

namespace pdr::experiments
{
  enum output_format {string, latex, markdown};
  struct Run
  {
    using Row_t   = tabulate::Table::Row_t;
    using Table_t = std::array<Row_t, 7>;
    std::string_view model;
    Tactic tactic;

    double avg_time;
    double std_dev_time;
    std::optional<Result::Invariant> max_inv;
    std::optional<Result::Trace> min_strat;

    Run(const my::cli::ArgumentList& args,
        const std::vector<ExperimentResults>& r);
    std::string str(output_format fmt) const;
    std::string str_compared(const Run& other, output_format fmt) const;

   private:
    Table_t listing() const;
    Table_t combined_listing(const Run& other) const;
  };

  void model_run(pebbling::Model& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
