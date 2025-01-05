#include "mclo/threading/mutex.hpp"

#ifdef _WIN32
#include <Windows.h>

namespace mclo
{
	static_assert( sizeof( SRWLOCK ) == sizeof( mclo::mutex ), "SRWLOCK size mismatch" );
	static_assert( alignof( SRWLOCK ) == alignof( mclo::mutex ), "SRWLOCK alignment mismatch" );

#define CAST_LOCK reinterpret_cast<PSRWLOCK>( &m_buffer )

	void mutex::lock() _Acquires_lock_( *CAST_LOCK )
	{
		AcquireSRWLockExclusive( CAST_LOCK );
	}

	bool mutex::try_lock() _When_( return != 0, _Acquires_exclusive_lock_( *CAST_LOCK ) )
	{
		return TryAcquireSRWLockExclusive( CAST_LOCK ) != 0;
	}

	void mutex::unlock() _Releases_lock_( *CAST_LOCK )
	{
		ReleaseSRWLockExclusive( CAST_LOCK );
	}

#undef CAST_LOCK
}
#endif
