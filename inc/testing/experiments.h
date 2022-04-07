#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

#include "pdr-context.h"

namespace pdr::experiments 
{
  void run(unsigned sample_size, Tactic tactic);
  void pdr_run(Tactic tactic);
} // namespace pdr::experiments

#endif // EXPERIMENTS_H
