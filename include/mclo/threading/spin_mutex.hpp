#pragma once

#include <atomic>

#include "mclo/threading/thread_pause.hpp"

namespace mclo
{
	/// @brief A lightweight mutex that busy-waits (spins) instead of blocking when contended.
	/// @details Implements the standard mutex named requirements using a single lock-free @c std::atomic_bool. While
	/// waiting it spins on a relaxed load and issues a CPU pause hint to avoid cache-line invalidation traffic and
	/// reduce power usage. This is well suited to very short critical sections where the cost of a kernel-level block
	/// would dominate, but it wastes CPU cycles and can cause priority inversion if the critical section is long or the
	/// lock is heavily contended; prefer a blocking mutex in those cases.
	/// @note Always profile before adopting a spin mutex to confirm it actually outperforms a blocking mutex for your
	/// workload; the right choice depends heavily on contention, critical section length, and core count.
	/// @see thread_pause
	class spin_mutex
	{
	public:
		constexpr spin_mutex() noexcept = default;

		/// @brief Acquires the lock, spinning until it becomes available.
		void lock() noexcept
		{
			while ( true )
			{
				// Attempt to acquire the lock
				if ( !m_lock.exchange( true, std::memory_order_acquire ) )
				{
					return;
				}

				// Waits for the lock to be released without causing cache invalidation, pauses for power efficiency
				while ( m_lock.load( std::memory_order_relaxed ) )
				{
					thread_pause();
				}
			}
		}

		/// @brief Releases the lock.
		void unlock() noexcept
		{
			m_lock.store( false, std::memory_order_release );
		}

		/// @brief Attempts to acquire the lock without spinning.
		/// @return @c true if the lock was acquired, @c false if it was already held.
		bool try_lock() noexcept
		{
			// First check without exchange to avoid unnecessary cache invalidation
			return !m_lock.load( std::memory_order_relaxed ) && !m_lock.exchange( true, std::memory_order_acquire );
		}

	private:
		static_assert( std::atomic_bool::is_always_lock_free, "spin_mutex requires lock-free atomic_bool" );

		std::atomic_bool m_lock{ false };
	};
}
