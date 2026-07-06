#pragma once

#include <cstddef>

namespace mclo
{
	/// @brief Returns the operating system's virtual memory page size in bytes.
	/// @details This is the granularity at which memory protection and paging operate, typically 4 KiB. The value
	/// is queried once from the operating system and cached for the lifetime of the process.
	/// @return The system page size in bytes.
	[[nodiscard]] std::size_t page_size() noexcept;

	/// @brief Returns the granularity at which virtual memory reservations and file mappings must be aligned.
	/// @details On Windows this is the allocation granularity, typically 64 KiB, required for the base address of
	/// @c VirtualAlloc reservations and @c MapViewOfFile views; on POSIX it is equal to @ref page_size. The value
	/// is queried once from the operating system and cached for the lifetime of the process.
	/// @return The memory allocation granularity in bytes.
	[[nodiscard]] std::size_t allocation_granularity() noexcept;
}
