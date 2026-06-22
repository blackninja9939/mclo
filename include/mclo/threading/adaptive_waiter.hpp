#pragma once

#include "mclo/threading/thread_pause.hpp"

#include <cstdint>

namespace mclo
{
	/// @brief A stack-local helper that adaptively backs off while busy-waiting in a spin loop.
	/// @details Construct one locally and call it repeatedly inside a retry loop, such as a CAS loop. Each call first
	/// spins with a CPU pause hint, doubling the number of pauses on successive calls, until a bounded total spin
	/// budget is used up; after that it yields the rest of the time slice to the scheduler. This keeps latency low
	/// under brief contention while avoiding burning a core when contention is sustained.
	/// @note The spin budget is a running total across calls, not a per-call count, so the entire spin phase performs
	/// at most @c max_spin_count pause hints before switching permanently to yielding.
	/// @warning State is not reset; construct a fresh instance for each independent wait loop.
	/// @see thread_pause
	class adaptive_waiter
	{
	public:
		constexpr adaptive_waiter() noexcept = default;

		/// @brief Performs a single adaptive backoff step, escalating from spinning to yielding over successive calls.
		void wait() noexcept
		{
			if ( m_spin_count_target <= max_spin_count )
			{
				do
				{
					thread_pause();
					++m_spin_count;
				}
				while ( m_spin_count < m_spin_count_target );
				m_spin_count_target *= 2;
			}
			else
			{
				yield();
			}
		}

	private:
		void yield() noexcept;

		static constexpr std::uint32_t max_spin_count = 1024;

		std::uint32_t m_spin_count = 0;
		std::uint32_t m_spin_count_target = 1;
	};
}
