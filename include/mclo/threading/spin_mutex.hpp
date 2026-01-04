#pragma once

#include <atomic>

#include "mclo/threading/thread_pause.hpp"

namespace mclo
{
	class spin_mutex
	{
	public:
		constexpr spin_mutex() noexcept = default;

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

		void unlock() noexcept
		{
			m_lock.store( false, std::memory_order_release );
		}

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
