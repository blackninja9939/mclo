#pragma once

#include <chrono>
#include <condition_variable>

#include "mclo/debug/assert.hpp"
#include "mclo/platform/os_detection.hpp"
#include "mclo/threading/mutex.hpp"

namespace mclo
{
	/// @brief A condition variable that is functionally identical to @c std::condition_variable but optimised for size
	/// on Windows.
	/// @details Provides the same interface and semantics as @c std::condition_variable, operating on @c mclo::mutex.
	/// On Windows it stores only a single pointer and uses a native condition variable directly. @c
	/// std::condition_variable is implemented on top of the very same primitive, but for ABI stability reasons its
	/// standard library type is padded out to a much larger size, so this type is a drop-in replacement with a smaller
	/// footprint. On all other platforms it is simply an alias for @c std::condition_variable.
#ifdef MCLO_OS_WINDOWS
	class condition_variable
	{
		// Windows timed waits must be less than INFINITE milliseconds, and waiting longer than this is not feasible in
		// practice, so longer requests are clamped to this maximum and then checked in a loop.
		static constexpr std::chrono::milliseconds max_wait_time{ std::chrono::hours{ 24 } };

		template <typename Rep, typename Period>
		[[nodiscard]] static constexpr unsigned long clamp_wait_time_to_ms(
			const std::chrono::duration<Rep, Period>& relative_time ) noexcept
		{
			if ( relative_time > max_wait_time )
			{
				return static_cast<unsigned long>( max_wait_time.count() );
			}
			else
			{
				const auto relative_ms = std::chrono::ceil<std::chrono::milliseconds>( relative_time );
				return static_cast<unsigned long>( relative_ms.count() );
			}
		}

	public:
		condition_variable() noexcept = default;
		~condition_variable() = default;

		void notify_one() noexcept;

		void notify_all() noexcept;

		void wait( std::unique_lock<mclo::mutex>& lock );

		template <typename Predicate>
		void wait( std::unique_lock<mclo::mutex>& lock, Predicate pred )
		{
			MCLO_DEBUG_ASSERT( lock.owns_lock(), "lock must be held" );
			while ( !pred() )
			{
				wait( lock );
			}
		}

		template <typename Rep, typename Period>
		std::cv_status wait_for( std::unique_lock<mclo::mutex>& lock,
								 const std::chrono::duration<Rep, Period> relative_time )
		{
			if ( relative_time <= std::chrono::duration<Rep, Period>::zero() )
			{
				return std::cv_status::timeout;
			}
			return wait_until( lock, std::chrono::steady_clock::now() + relative_time );
		}

		template <typename Rep, typename Period, typename Predicate>
		bool wait_for( std::unique_lock<mclo::mutex>& lock,
					   const std::chrono::duration<Rep, Period> relative_time,
					   Predicate pred )
		{
			return wait_until( lock, std::chrono::steady_clock::now() + relative_time, pred );
		}

		template <typename Clock, typename Duration>
		std::cv_status wait_until( std::unique_lock<mclo::mutex>& lock,
								   const std::chrono::time_point<Clock, Duration> absolute_time )
		{
			static_assert( std::chrono::is_clock_v<Clock>, "Clock must be a clock type" );
			MCLO_DEBUG_ASSERT( lock.owns_lock(), "lock must be held" );
			// Loop because the wait time is clamped to a maximum, so a single timed wait may expire before the real
			// deadline. Each iteration waits for the clamped remaining time until the deadline is genuinely reached.
			for ( ;; )
			{
				const auto now = Clock::now();
				if ( absolute_time <= now )
				{
					return std::cv_status::timeout;
				}

				// A successful wait means we were woken, either by a notification or a spurious OS wakeup.
				if ( wait_for_ms( lock, clamp_wait_time_to_ms( absolute_time - now ) ) )
				{
					return std::cv_status::no_timeout;
				}
			}
		}

		template <typename Clock, typename Duration, typename Predicate>
		bool wait_until( std::unique_lock<mclo::mutex>& lock,
						 const std::chrono::time_point<Clock, Duration> absolute_time,
						 Predicate pred )
		{
			static_assert( std::chrono::is_clock_v<Clock>, "Clock must be a clock type" );
			MCLO_DEBUG_ASSERT( lock.owns_lock(), "lock must be held" );
			while ( !pred() )
			{
				if ( wait_until( lock, absolute_time ) == std::cv_status::timeout )
				{
					return pred();
				}
			}
			return true;
		}

	private:
		bool wait_for_ms( std::unique_lock<mclo::mutex>& lock, unsigned long ms );

		alignas( void* ) std::byte m_buffer[ sizeof( void* ) ] = {};
	};
#else
	using std::condition_variable;
#endif
}
