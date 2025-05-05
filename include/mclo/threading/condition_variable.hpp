#pragma once

#include <condition_variable>

#include "mclo/threading/mutex.hpp"

namespace mclo
{
#ifdef _WIN32
	class condition_variable
	{
		template <typename Rep, typename Period>
		[[nodiscard]] static constexpr unsigned long clamp_wait_time_to_ms(
			const std::chrono::duration<Rep, Period>& relative_time ) noexcept
		{
			// Must Clamp so that relative_time is less than Windows INFINITE milliseconds.
			constexpr std::chrono::milliseconds clamp{ std::chrono::hours{ 24 } };

			if ( relative_time > clamp )
			{
				return static_cast<unsigned long>( clamp.count() );
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
			while ( !pred() )
			{
				wait( lock );
			}
		}

		template <typename Rep, typename Period>
		std::cv_status wait_for( std::unique_lock<mclo::mutex>& lock,
								 const std::chrono::duration<Rep, Period>& relative_time )
		{
			if ( wait_for_ms( lock, clamp_wait_time_to_ms( relative_time ) ) )
			{
				return std::cv_status::no_timeout;
			}
			else
			{
				return std::cv_status::timeout;
			}
		}

		template <typename Rep, typename Period, typename Predicate>
		bool wait_for( std::unique_lock<std::mutex>& lock,
					   const std::chrono::duration<Rep, Period>& relative_time,
					   Predicate pred )
		{
			return wait_until( lock, std::chrono::steady_clock::now() + relative_time, pred );
		}

		template <typename Clock, typename Duration>
		std::cv_status wait_until( std::unique_lock<mclo::mutex>& lock,
								   const std::chrono::time_point<Clock, Duration>& absolute_time )
		{
			const auto now = Clock::now();
			if ( now >= absolute_time )
			{
				return std::cv_status::timeout;
			}
			else
			{
				return wait_for( lock, absolute_time - now );
			}
		}

		template <typename Clock, typename Duration, typename Predicate>
		bool wait_until( std::unique_lock<std::mutex>& lock,
						 const std::chrono::time_point<Clock, Duration>& absolute_time,
						 Predicate pred )
		{
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
