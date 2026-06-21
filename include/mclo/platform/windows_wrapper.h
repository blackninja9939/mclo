#pragma once

#include "mclo/platform/os_detection.hpp"

#ifdef MCLO_OS_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <system_error>

namespace mclo
{
	/// @brief Returns an @c std::error_code describing the calling thread's last Windows error.
	/// @details Wraps @c GetLastError and maps it onto the system error category.
	/// @return An error code for the most recent failed Windows API call on the calling thread.
	[[nodiscard]] std::error_code last_error_code() noexcept;
}

#endif
