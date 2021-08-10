#include "pdr.h"

namespace pdr 
{
	void PDR::decrement(int x)
	{
		int max_pebbles = model.get_max_pebbles();
		assert(x < max_pebbles);

		model.set_max_pebbles(max_pebbles - x);
	}

	void PDR::store_frames()
	{
		old_frames = std::move(frames);
		frames.clear();
		std::cout << "Stored old frames for next run" << std::endl;
	}
}
