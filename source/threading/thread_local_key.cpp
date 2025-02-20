#include "mclo/threading/thread_local_key.hpp"

#include "mclo/debug/assert.hpp"

#include <stdexcept>

#ifdef _WIN32
#include "windows_wrapper.h"

namespace mclo
{
	static_assert( sizeof( thread_local_key::native_handle_type ) == sizeof( DWORD ),
				   "DWORD is not std::uint32_t size is not the correct size" );

	thread_local_key::thread_local_key()
		: m_key( TlsAlloc() )
	{
		if ( m_key == TLS_OUT_OF_INDEXES )
		{
			throw std::bad_alloc();
		}
	}

	thread_local_key::~thread_local_key()
	{
		[[maybe_unused]] const BOOL result = TlsFree( m_key );
		DEBUG_ASSERT( result != 0, "Failed to free thread local key" );
	}

	void thread_local_key::set( void* value )
	{
		[[maybe_unused]] const BOOL result = TlsSetValue( m_key, value );
		DEBUG_ASSERT( result != 0, "Failed to set thread local value" );
	}

	void* thread_local_key::get() const noexcept
	{
		return TlsGetValue( m_key );
	}
}
#else
namespace mclo
{
	thread_local_key::thread_local_key()
	{
		if ( pthread_key_create( &m_key, nullptr ) != 0 )
		{
			throw std::bad_alloc();
		}
	}

	thread_local_key::~thread_local_key()
	{
		[[maybe_unused]] const int result == pthread_key_delete( m_key );
		DEBUG_ASSERT( result == 0, "Failed to delete thread local key" );
	}

	void thread_local_key::set( void* value )
	{
		[[maybe_unused]] const int result = pthread_setspecific( m_key, value );
		DEBUG_ASSERT( result == 0, "Failed to set thread local value" );
	}

	void* thread_local_key::get() const noexcept
	{
		return pthread_getspecific( m_key );
	}
}
#endif
