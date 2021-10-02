#include "pdr.h"
#include <bits/types/FILE.h>
#include <cstddef>

namespace pdr 
{
	void PDR::decrement(int x)
	{
		assert(frames.size() > 0);
		int max_pebbles = model.get_max_pebbles();
		assert(x < max_pebbles);

		model.set_max_pebbles(max_pebbles - x);
		for (size_t i = 0; i < frames.size(); i++)
			frames[i]->reset_frame(stats, { model.property.currents(), model.get_transition(), model.get_cardinality() });

		string msg = "Dynamic: skip initiation. k = " + std::to_string(k);
		std::cout << msg << std::endl;
		SPDLOG_LOGGER_INFO(log, msg);
		//if we are repeating, the last propagation was k-1, repeat this
		propagate(k-1);
	}

	void PDR::store_frames()
	{
		// old_frames = std::move(frames);
		// frames.clear();
		std::cout << "Stored old frames for next run" << std::endl;
	}

}
