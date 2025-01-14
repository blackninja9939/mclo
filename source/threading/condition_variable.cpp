#include "mclo/threading/condition_variable.hpp"

#ifdef _WIN32
#include "windows_wrapper.h"

#include "mclo/debug/assert.hpp"

namespace mclo
{
	static_assert( sizeof( CONDITION_VARIABLE ) == sizeof( mclo::condition_variable ),
				   "CONDITION_VARIABLE size mismatch" );
	static_assert( alignof( CONDITION_VARIABLE ) == alignof( mclo::condition_variable ),
				   "CONDITION_VARIABLE alignment mismatch" );

#define CAST_COND_VAR reinterpret_cast<PCONDITION_VARIABLE>( &m_buffer )
#define CAST_LOCK( BUFF ) reinterpret_cast<PSRWLOCK>( BUFF )

	void condition_variable::notify_one() noexcept
	{
		WakeConditionVariable( CAST_COND_VAR );
	}

	void condition_variable::notify_all() noexcept
	{
		WakeAllConditionVariable( CAST_COND_VAR );
	}

	void condition_variable::wait( std::unique_lock<mclo::mutex>& lock )
	{
		if ( !wait_for_ms( lock, INFINITE ) )
		{
			std::abort();
		}
	}

	bool condition_variable::wait_for_ms( std::unique_lock<mclo::mutex>& lock, unsigned long ms )
	{
		static_assert( std::is_same_v<unsigned long, DWORD>, "DWORD must be the same as unsigned long" );
		DEBUG_ASSERT( lock.owns_lock(), "lock must be held" );
		return SleepConditionVariableSRW( CAST_COND_VAR, CAST_LOCK( &lock.mutex()->m_buffer ), ms, 0 ) != 0;
	}

#undef CAST_COND_VAR
#undef CAST_LOCK
}
#endif
