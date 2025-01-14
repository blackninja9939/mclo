#include "mclo/threading/thread_properties.hpp"

#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_map.hpp"

template <>
constexpr mclo::thread_priority mclo::enum_size<mclo::thread_priority> = mclo::thread_priority::high;

#ifdef _WIN32

#include <limits>
#include <string>

#include "windows_wrapper.h"

void mclo::set_thread_name( std::thread& thread, const std::string_view name )
{
	const int name_size = static_cast<int>( name.size() );
	const int wide_size = MultiByteToWideChar( CP_UTF8, 0, name.data(), name_size, nullptr, 0 );
	DEBUG_ASSERT( wide_size > 0, "Failed to convert name to wide string", GetLastError() );

	std::wstring wide( wide_size, {} );
	[[maybe_unused]] const int convert_result =
		MultiByteToWideChar( CP_UTF8, 0, name.data(), name_size, wide.data(), wide_size );
	DEBUG_ASSERT( convert_result > 0, "Failed to convert name to wide string", GetLastError() );

	[[maybe_unused]] const HRESULT name_result =
		SetThreadDescription( static_cast<HANDLE>( thread.native_handle() ), wide.c_str() );
	DEBUG_ASSERT( SUCCEEDED( name_result ), "Failed to set thread name" );
}

void mclo::set_thread_priority( std::thread& thread, const thread_priority priority )
{
	static constexpr mclo::enum_map<thread_priority, int> prio_map{
		THREAD_PRIORITY_LOWEST,
		THREAD_PRIORITY_BELOW_NORMAL,
		THREAD_PRIORITY_NORMAL,
		THREAD_PRIORITY_ABOVE_NORMAL,
		THREAD_PRIORITY_HIGHEST,
	};
	[[maybe_unused]] const BOOL result =
		SetThreadPriority( static_cast<HANDLE>( thread.native_handle() ), prio_map[ priority ] );
	DEBUG_ASSERT( result != 0, "Failed to set thread priority" );
}

void mclo::set_thread_affinity( std::thread& thread, const std::uint64_t affinity )
{
	const DWORD_PTR result = SetThreadAffinityMask( thread.native_handle(), affinity );
	DEBUG_ASSERT( result == affinity, "Fauked to set thread affinity", GetLastError() );
}

#else

#include <cstring>
#include <pthread.h>

void mclo::set_thread_name( std::thread& thread, const std::string_view name )
{
	char truncated[ 16 ];
	std::strncpy( truncated, name.data(), 15 );
	const int result = pthread_setname_np( thread.native_handle(), truncated );
	DEBUG_ASSERT( result == 0, "Failed to set thread name" );
}

void mclo::set_thread_priority( std::thread& thread, const thread_priority priority )
{
	static constexpr int normal_prio = 5;
	static constexpr mclo::enum_map<thread_priority, int> prio_map{
		normal_prio - 2,
		normal_prio - 1,
		normal_prio,
		normal_prio + 1,
		normal_prio + 2,
	};

	sched_param sch;
	int policy;
	pthread_getschedparam( thread.native_handle(), &policy, &sch );
	sch.sched_priority = static_cast<int>( priority );
	pthread_setschedparam( thread.native_handle(), policy, &sch );
}

void mclo::set_thread_affinity( std::thread&, const std::uint64_t )
{
}

#endif
