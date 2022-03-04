#include "pdr.h"
#include <bits/types/FILE.h>
#include <cstddef>
#include <string>

namespace pdr
{
  bool PDR::decrement(bool reuse)
  {
    int max_pebbles = ctx.const_model().get_max_pebbles();
    int new_pebbles = shortest_strategy - 1;
    assert(new_pebbles > 0);
    assert(new_pebbles < max_pebbles);

    if (!ctx.model().set_max_pebbles(new_pebbles))
      return false;

    reset();
    results.extend();
    logger.whisper() << "retrying with " << new_pebbles << std::endl;
    if (!reuse)
      return true;

    // TODO separate staistics from dyn runs?
    frames.reset_frames(logger.stats,
                        { ctx.const_model().property.currents(), ctx.const_model().get_transition(),
                          ctx.const_model().get_cardinality() });

    log_and_show("Dynamic: skip initiation. k = " + std::to_string(k));
    // if we are repeating, the last propagation was k-1, repeat this
    int invariant = frames.propagate(k - 1, true);
    if (invariant >= 0)
    {
      results.current().invariant_index = invariant;
      return true;
    }
    return false;
  }
} // namespace pdr
