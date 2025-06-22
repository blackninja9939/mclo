#include "mclo/threading/thread_properties.hpp"

#include "mclo/enum/enum_size.hpp"

template <>
constexpr mclo::thread_priority mclo::enum_size<mclo::thread_priority> = static_cast<mclo::thread_priority>(
	static_cast<std::underlying_type_t<mclo::thread_priority>>( mclo::thread_priority::high ) + 1 );

#ifdef _WIN32

#include <limits>
#include <string>

#include "windows_wrapper.h"

#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_map.hpp"

namespace
{
	void set_thread_name_platform( std::thread::native_handle_type thread, const std::string_view name )
	{
		const int name_size = static_cast<int>( name.size() );
		const int wide_size = MultiByteToWideChar( CP_UTF8, 0, name.data(), name_size, nullptr, 0 );
		DEBUG_ASSERT( wide_size > 0, "Failed to convert name to wide string", GetLastError() );

		std::wstring wide( wide_size, {} );
		[[maybe_unused]] const int convert_result =
			MultiByteToWideChar( CP_UTF8, 0, name.data(), name_size, wide.data(), wide_size );
		DEBUG_ASSERT( convert_result > 0, "Failed to convert name to wide string", GetLastError() );

		[[maybe_unused]] const HRESULT name_result =
			SetThreadDescription( static_cast<HANDLE>( thread ), wide.c_str() );
		DEBUG_ASSERT( SUCCEEDED( name_result ), "Failed to set thread name" );
	}

	void set_thread_priority_platform( std::thread::native_handle_type thread, const mclo::thread_priority priority )
	{
		static constexpr mclo::enum_map<mclo::thread_priority, int> prio_map{
			mclo::sorted_unique,
			THREAD_PRIORITY_LOWEST,
			THREAD_PRIORITY_BELOW_NORMAL,
			THREAD_PRIORITY_NORMAL,
			THREAD_PRIORITY_ABOVE_NORMAL,
			THREAD_PRIORITY_HIGHEST,
		};
		[[maybe_unused]] const BOOL result = SetThreadPriority( static_cast<HANDLE>( thread ), prio_map[ priority ] );
		DEBUG_ASSERT( result != 0, "Failed to set thread priority" );
	}

	void set_thread_affinity_platform( std::thread::native_handle_type thread, const std::uint64_t affinity )
	{
		[[maybe_unused]] const DWORD_PTR result = SetThreadAffinityMask( thread, affinity );
		DEBUG_ASSERT( result == affinity, "Failed to set thread affinity", GetLastError() );
	}
}

#elif defined( __linux__ )

#include <cstring>
#include <pthread.h>

#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_map.hpp"

namespace
{
	void mclo::set_thread_name( std::thread::native_handle_type thread, const std::string_view name )
	{
		char truncated[ 16 ];
		std::strncpy( truncated, name.data(), 15 );
		const int result = pthread_setname_np( thread, truncated );
		DEBUG_ASSERT( result == 0, "Failed to set thread name" );
	}

	void set_thread_priority_platform( std::thread::native_handle_type thread, const mclo::thread_priority priority )
	{
		static constexpr int normal_prio = 5;
		static constexpr mclo::enum_map<mclo::thread_priority, int> prio_map{
			mclo::sorted_unique,
			normal_prio - 2,
			normal_prio - 1,
			normal_prio,
			normal_prio + 1,
			normal_prio + 2,
		};

		sched_param sch;
		int policy;
		pthread_getschedparam( thread, &policy, &sch );
		sch.sched_priority = static_cast<int>( priority );
		pthread_setschedparam( thread, policy, &sch );
	}

	void set_thread_affinity_platform( std::thread::native_handle_type, const std::uint64_t )
	{
	}
}

#else

namespace
{
	void set_thread_name_platform( std::thread::native_handle_type, const std::string_view )
	{
	}

	void set_thread_priority_platform( std::thread::native_handle_type, const thread_priority )
	{
	}

	void set_thread_affinity_platform( std::thread::native_handle_type, const std::uint64_t )
	{
	}
}

#endif

void mclo::set_thread_name( std::thread& thread, const std::string_view name )
{
	set_thread_name_platform( thread.native_handle(), name );
}

void mclo::set_thread_name( std::jthread& thread, const std::string_view name )
{
	set_thread_name_platform( thread.native_handle(), name );
}

void mclo::set_thread_priority( std::thread& thread, const thread_priority priority )
{
	set_thread_priority_platform( thread.native_handle(), priority );
}

void mclo::set_thread_priority( std::jthread& thread, const thread_priority priority )
{
	set_thread_priority_platform( thread.native_handle(), priority );
}

void mclo::set_thread_affinity( std::thread& thread, const std::uint64_t affinity )
{
	set_thread_affinity_platform( thread.native_handle(), affinity );
}

void mclo::set_thread_affinity( std::jthread& thread, const std::uint64_t affinity )
{
	set_thread_affinity_platform( thread.native_handle(), affinity );
}
