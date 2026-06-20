#pragma once

#include <cinttypes>
#include <string_view>
#include <thread>

namespace mclo
{
	/// @brief Relative scheduling priorities that can be assigned to a thread.
	enum class thread_priority : std::uint8_t
	{
		/// @brief The lowest available scheduling priority.
		lowest,
		/// @brief A scheduling priority below normal.
		low,
		/// @brief The default scheduling priority.
		normal,
		/// @brief A scheduling priority above normal.
		elevated,
		/// @brief The highest available scheduling priority.
		high,
	};

	/// @brief Sets the name of the given thread for display in debuggers and profilers.
	/// @param handle The native handle of the thread to name.
	/// @param name The name to assign to the thread.
	void set_thread_name( std::thread::native_handle_type handle, const std::string_view name );

	/// @brief Sets the name of the calling thread for display in debuggers and profilers.
	/// @param name The name to assign to the current thread.
	void set_current_thread_name( const std::string_view name );

	/// @brief Sets the scheduling priority of the given thread.
	/// @param handle The native handle of the thread to modify.
	/// @param priority The priority to assign to the thread.
	void set_thread_priority( std::thread::native_handle_type handle, const thread_priority priority );

	/// @brief Sets the scheduling priority of the calling thread.
	/// @param priority The priority to assign to the current thread.
	void set_current_thread_priority( const thread_priority priority );

	/// @brief Sets the CPU affinity mask of the given thread.
	/// @param handle The native handle of the thread to modify.
	/// @param affinity A bitmask where each set bit permits the thread to run on the corresponding logical CPU.
	void set_thread_affinity( std::thread::native_handle_type handle, const std::uint64_t affinity );

	/// @brief Sets the CPU affinity mask of the calling thread.
	/// @param affinity A bitmask where each set bit permits the thread to run on the corresponding logical CPU.
	void set_current_thread_affinity( const std::uint64_t affinity );
}
