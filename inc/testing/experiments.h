#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "logger.h"
#include "pdr-context.h"
#include "pdr.h"
#include "result.h"
#include <variant>

namespace pdr::experiments
{
  struct Run
  {
    std::string model;
    double avg_time;
    unsigned constraint;
    std::optional<Result::Invariant> max_inv;
    std::optional<Result::Trace> min_strat;

    Run(std::string_view m, const std::vector<ExperimentResults>& r);
  };

  void model_run(pebbling::Model& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
