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

	void set_thread_name( std::thread::native_handle_type handle, const std::string_view name );
	void set_current_thread_name( const std::string_view name );

	void set_thread_priority( std::thread::native_handle_type handle, const thread_priority priority );
	void set_current_thread_priority( const thread_priority priority );

	void set_thread_affinity( std::thread::native_handle_type handle, const std::uint64_t affinity );
	void set_current_thread_affinity( const std::uint64_t affinity );
}
