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
	[[nodiscard]] std::error_code last_error_code() noexcept;
}

#endif
