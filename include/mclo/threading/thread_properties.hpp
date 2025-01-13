#pragma once

#include <cinttypes>
#include <string_view>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace mclo
{
	enum class thread_priority : int
	{
#ifdef _WIN32
		High = THREAD_PRIORITY_HIGHEST,
		Elevated = THREAD_PRIORITY_ABOVE_NORMAL,
		Normal = THREAD_PRIORITY_NORMAL,
		Low = THREAD_PRIORITY_BELOW_NORMAL,
		Lowest = THREAD_PRIORITY_LOWEST,
#else
		Normal = 5,
		High = Normal + 2,
		Elevated = Normal + 1,
		Low = Normal - 1,
		Lowest = Normal - 2,
#endif
	};

	void set_thread_name( std::thread& thread, const std::string_view name );
	void set_thread_priority( std::thread& thread, const thread_priority priority );
	void set_thread_affinity( std::thread& thread, const std::uint64_t affinity );
}
