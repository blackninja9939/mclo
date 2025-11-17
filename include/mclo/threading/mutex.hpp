#pragma once

#include <mutex>

#include "mclo/platform/os_detection.hpp"

namespace mclo
{
#ifdef MCLO_OS_WINDOWS
	class [[nodiscard]] mutex
	{
	public:
		friend class condition_variable;

		constexpr mutex() noexcept = default;
		~mutex() = default;

		mutex( const mutex& ) = delete;
		mutex& operator=( const mutex& ) = delete;

		_Acquires_lock_( m_buffer ) void lock();
		_When_( return != 0, _Acquires_lock_( m_buffer ) ) [[nodiscard]] bool try_lock();
		_Releases_lock_( m_buffer ) void unlock();

	private:
		alignas( void* ) std::byte m_buffer[ sizeof( void* ) ] = {};
	};
#else
	using std::mutex;
#endif
}
