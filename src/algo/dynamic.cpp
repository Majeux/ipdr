#include "pdr.h"
#include <bits/types/FILE.h>
#include <cstddef>
#include <string>

namespace pdr
{
  bool PDR::decrement()
  {
    int max_pebbles = model.get_max_pebbles();
    assert(shortest_strategy > 0);
    int new_pebbles = shortest_strategy - 1;
    assert(new_pebbles < max_pebbles);

    model.set_max_pebbles(new_pebbles);
    // TODO separate staistics from dyn runs
    frames.reset_frames(logger.stats,
                        {model.property.currents(), model.get_transition(),
                         model.get_cardinality()});

    log_and_show("Dynamic: skip initiation. k = " + std::to_string(k));
    // if we are repeating, the last propagation was k-1, repeat this
    int invariant = frames.propagate(k - 1, true);
    if (invariant >= 0)
    {
      result().invariant_index = invariant;
      return true;
    }
    return false;
  }
} // namespace pdr
