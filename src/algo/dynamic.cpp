#include "pdr.h"

namespace pdr 
{
	void PDR::store_frames()
	{
		old_frames = std::move(frames);
		frames.clear();
	}
}
