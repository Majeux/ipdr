#include "pdr.h"
#include <bits/types/FILE.h>
#include <cstddef>

namespace pdr 
{
	void PDR::decrement(int x)
	{
		int max_pebbles = model.get_max_pebbles();
		assert(x < max_pebbles);

		model.set_max_pebbles(max_pebbles - x);
		frames.reset_frames(logger.stats, //TODO separate staistics from dyn runs
				{ model.property.currents(), model.get_transition(), model.get_cardinality() });

		string msg = "Dynamic: skip initiation. k = " + std::to_string(k);
		std::cout << msg << std::endl;
		SPDLOG_LOGGER_INFO(logger.spd_logger, msg);
		//if we are repeating, the last propagation was k-1, repeat this
		frames.propagate(k-1, true);
	}
}
