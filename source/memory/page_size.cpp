#include "mclo/memory/page_size.hpp"

#include "mclo/platform/os_detection.hpp"

#ifdef MCLO_OS_WINDOWS
#include "mclo/platform/windows_wrapper.hpp"
#else
#include <unistd.h>
#endif

namespace
{
	struct memory_granularity
	{
		std::size_t page_size = 0;
		std::size_t allocation_granularity = 0;
	};

	memory_granularity query_memory_granularity() noexcept
	{
#ifdef MCLO_OS_WINDOWS
		SYSTEM_INFO info;
		GetSystemInfo( &info );
		return { info.dwPageSize, info.dwAllocationGranularity };
#else
		const auto size = static_cast<std::size_t>( sysconf( _SC_PAGESIZE ) );
		return { size, size };
#endif
	}

	const memory_granularity granularity = query_memory_granularity();
}

std::size_t mclo::page_size() noexcept
{
	return granularity.page_size;
}

std::size_t mclo::allocation_granularity() noexcept
{
	return granularity.allocation_granularity;
}
