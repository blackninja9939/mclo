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
		high = THREAD_PRIORITY_HIGHEST,
		elevated = THREAD_PRIORITY_ABOVE_NORMAL,
		normal = THREAD_PRIORITY_NORMAL,
		low = THREAD_PRIORITY_BELOW_NORMAL,
		lowest = THREAD_PRIORITY_LOWEST,
#else
		normal = 5,
		high = normal + 2,
		elevated = normal + 1,
		low = normal - 1,
		lowest = normal - 2,
#endif
	};

	void set_thread_name( std::thread& thread, const std::string_view name );
	void set_thread_priority( std::thread& thread, const thread_priority priority );
	void set_thread_affinity( std::thread& thread, const std::uint64_t affinity );
}
