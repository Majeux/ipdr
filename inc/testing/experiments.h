#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "logger.h"
#include "pdr-context.h"
#include "pdr.h"
#include <variant>

namespace pdr::experiments 
{
  // clang-format off
  struct Invariant { int index; };
  struct Trace { unsigned length; unsigned pebbles; };
  // clang-format on
  struct Result
  {
    unsigned cardinality;
    std::variant<Invariant, Trace> result;
    double time;
  };

  struct Run 
  {
    Tactic tactic;
    bool delta;
    unsigned sample_size;
    std::optional<unsigned> n_pebbles;

    Run(Tactic t, bool d, unsigned ss, std::optional<unsigned> p = {});
  };

  void model_run(pdr::PebblingModel& model, pdr::Logger& log, unsigned sample_size, Tactic tactic);
  void pdr_run(pdr::PDR& alg, Tactic tactic);
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
