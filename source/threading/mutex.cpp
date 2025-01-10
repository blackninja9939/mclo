#include "mclo/threading/mutex.hpp"

#ifdef _WIN32
#include <Windows.h>

namespace mclo
{
	static_assert( sizeof( SRWLOCK ) == sizeof( mclo::mutex ), "SRWLOCK size mismatch" );
	static_assert( alignof( SRWLOCK ) == alignof( mclo::mutex ), "SRWLOCK alignment mismatch" );

#define CAST_LOCK reinterpret_cast<PSRWLOCK>( &m_buffer )

	_Acquires_lock_( m_buffer ) void mutex::lock()
	{
		AcquireSRWLockExclusive( CAST_LOCK );
	}

	_When_( return != 0, _Acquires_exclusive_lock_( m_buffer ) ) bool mutex::try_lock()
	{
		return TryAcquireSRWLockExclusive( CAST_LOCK ) != 0;
	}

	_Releases_lock_( m_buffer ) void mutex::unlock()
	{
		ReleaseSRWLockExclusive( CAST_LOCK );
	}

#undef CAST_LOCK
}
#endif
