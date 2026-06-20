#pragma once

#include <mutex>

#include "mclo/platform/os_detection.hpp"

namespace mclo
{
	/// @brief A mutex that is functionally identical to @c std::mutex but optimised for size on Windows.
	/// @details Provides the same interface and semantics as @c std::mutex. On Windows it stores only a single pointer
	/// and uses a Slim Reader/Writer (SRW) lock directly. @c std::mutex is implemented on top of the very same SRW
	/// lock, but for ABI stability reasons its standard library type is padded out to a much larger size, so this type
	/// is a drop-in replacement with a smaller footprint. On all other platforms it is simply an alias for @c
	/// std::mutex.
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
