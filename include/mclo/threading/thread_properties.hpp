#pragma once

#include <cinttypes>
#include <string_view>
#include <thread>

namespace mclo
{
	enum class thread_priority : std::uint8_t
	{
		lowest,
		low,
		normal,
		elevated,
		high,
	};

	void set_thread_name( std::thread& thread, const std::string_view name );
	void set_thread_name( std::jthread& thread, const std::string_view name );

	void set_thread_priority( std::thread& thread, const thread_priority priority );
	void set_thread_priority( std::jthread& thread, const thread_priority priority );
	
	void set_thread_affinity( std::thread& thread, const std::uint64_t affinity );
	void set_thread_affinity( std::jthread& thread, const std::uint64_t affinity );

}
