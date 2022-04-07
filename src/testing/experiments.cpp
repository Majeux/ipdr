#include "experiments.h"

namespace pdr::experiments 
{

  void run(unsigned sample_size, Tactic tactic) 
  {
    for (unsigned i = 0; i < sample_size; i++)
    {
      pdr_run(tactic);
    }
  }

  void pdr_run(Tactic tactic)
  {
    
  }
  
} // namespace pdr::experiments
