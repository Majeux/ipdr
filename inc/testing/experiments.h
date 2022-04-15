#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "logger.h"
#include "pdr-context.h"
#include "pdr.h"
#include <variant>

namespace pdr::experiments
{
  struct ExperimentResult
  {
  };
  struct Run
  {
    Tactic tactic;
    bool delta;
    unsigned sample_size;
    std::optional<unsigned> n_pebbles;

    Run(Tactic t, bool d, unsigned ss, std::optional<unsigned> p = {});
  };

  void model_run(pebbling::Model& model, pdr::Logger& log,
      const my::cli::ArgumentList& args);
  ExperimentResult pdr_run(pdr::PDR& alg, Tactic tactic);
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
