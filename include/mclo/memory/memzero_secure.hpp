#pragma once

#include <cstddef>

namespace mclo
{
	/// @brief Securely zeroes a region of memory in a way that will not be elided by the optimiser.
	/// @details Ordinary @c std::memset or @c std::fill calls that write to memory which is never read again are a
	/// dead store and may be removed by the compiler. This function guarantees the write happens, making it suitable
	/// for scrubbing sensitive data such as keys or passwords before the storage is released or reused.
	/// @param ptr Pointer to the start of the region to zero. Must point to at least @p size writable bytes.
	/// @param size Number of bytes to zero.
	void memzero_secure( void* ptr, std::size_t size ) noexcept;
}
