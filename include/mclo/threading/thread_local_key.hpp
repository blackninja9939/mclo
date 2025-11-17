#pragma once

#include <cinttypes>

#include "mclo/platform/os_detection.hpp"

#ifndef MCLO_OS_WINDOWS
#include <pthread.h>
#endif

namespace mclo
{
	class thread_local_key
	{
	public:
#ifdef MCLO_OS_WINDOWS
		using native_handle_type = std::uint32_t;
#else
		using native_handle_type = pthread_key_t;
#endif
		thread_local_key();
		~thread_local_key();

		thread_local_key( const thread_local_key& ) = delete;
		thread_local_key( thread_local_key&& ) = delete;
		thread_local_key& operator=( const thread_local_key& ) = delete;
		thread_local_key& operator=( thread_local_key&& ) = delete;

		void set( void* value );
		void* get() const noexcept;

		native_handle_type native_handle() const noexcept
		{
			return m_key;
		}

	private:
		native_handle_type m_key;
	};
}
