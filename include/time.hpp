#pragma once

#include <chrono>

namespace rotm::time
{
	using Clock = std::chrono::steady_clock;
	using Duration = std::chrono::steady_clock::duration;
	using Point = std::chrono::steady_clock::time_point;

	inline time::Duration since(time::Point timePoint)
	{
		return Clock::now() - timePoint;
	}
} // namespace rotm::time
