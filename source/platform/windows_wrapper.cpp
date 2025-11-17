#include "mclo/platform/windows_wrapper.h"

#ifdef MCLO_OS_WINDOWS

std::error_code mclo::last_error_code() noexcept
{
	return std::error_code{ static_cast<int>( GetLastError() ), std::system_category() };
}

#endif
