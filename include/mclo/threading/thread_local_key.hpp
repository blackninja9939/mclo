#pragma once

#include <cinttypes>

#include "mclo/platform/os_detection.hpp"

#ifndef MCLO_OS_WINDOWS
#include <pthread.h>
#endif

namespace mclo
{
	/// @brief An RAII wrapper around a platform thread-local storage (TLS) slot holding a single @c void* per thread.
	/// @details Provides dynamically allocated thread-local storage, unlike the @c thread_local keyword which requires
	/// a static declaration. Each thread that accesses the key sees its own independent pointer value, default
	/// initialised to @c nullptr. The underlying slot is allocated on construction and released on destruction. Backed
	/// by Win32 TLS
	/// (@c TlsAlloc) on Windows and POSIX thread-specific data (@c pthread_key_t) elsewhere.
	/// @note No destructor callback is registered for stored values; the user is responsible for managing the lifetime
	/// of anything the stored pointer refers to.
	class thread_local_key
	{
	public:
#ifdef MCLO_OS_WINDOWS
		using native_handle_type = std::uint32_t;
#else
		using native_handle_type = pthread_key_t;
#endif
		/// @brief Allocates a new thread-local storage slot.
		thread_local_key();

		/// @brief Releases the thread-local storage slot.
		~thread_local_key();

		thread_local_key( const thread_local_key& ) = delete;
		thread_local_key( thread_local_key&& ) = delete;
		thread_local_key& operator=( const thread_local_key& ) = delete;
		thread_local_key& operator=( thread_local_key&& ) = delete;

		/// @brief Sets the value stored for the calling thread.
		/// @param value The pointer to store for the current thread.
		void set( void* value );

		/// @brief Retrieves the value stored for the calling thread.
		/// @return The pointer previously stored by this thread, or @c nullptr if none has been set.
		void* get() const noexcept;

		/// @brief Returns the underlying platform-specific handle for the storage slot.
		/// @return The native TLS handle.
		native_handle_type native_handle() const noexcept
		{
			return m_key;
		}

	private:
		native_handle_type m_key;
	};
}
