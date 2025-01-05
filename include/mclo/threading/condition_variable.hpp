#pragma once

#include <condition_variable>

#include "mclo/threading/mutex.hpp"

namespace mclo
{
#ifdef _WIN32
	class condition_variable
	{
	public:
		condition_variable() noexcept = default;
		~condition_variable() = default;

		void notify_one() noexcept;

		void notify_all() noexcept;

		void wait( std::unique_lock<mclo::mutex>& lock );

		template <class Predicate>
		void wait( std::unique_lock<mclo::mutex>& lock, Predicate pred )
		{
			while ( !pred() )
			{
				wait( lock );
			}
		}

		template <class Rep, class Period>
		std::cv_status wait_for( std::unique_lock<mclo::mutex>& lock,
								 const std::chrono::duration<Rep, Period>& rel_time )
		{
			const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>( rel_time ).count();
			if ( wait_for_ms( lock, ms ) )
			{
				return std::cv_status::no_timeout;
			}
			else
			{
				return std::cv_status::timeout;
			}
		}

		template <class Rep, class Period, class Predicate>
		bool wait_for( std::unique_lock<std::mutex>& lock,
					   const std::chrono::duration<Rep, Period>& rel_time,
					   Predicate pred )
		{
			return wait_until( lock, std::chrono::steady_clock::now() + rel_time, pred );
		}

		template <class Clock, class Duration>
		std::cv_status wait_until( std::unique_lock<mclo::mutex>& lock,
								   const std::chrono::time_point<Clock, Duration>& abs_time )
		{
			const auto now = Clock::now();
			if ( now >= abs_time )
			{
				return std::cv_status::timeout;
			}
			else
			{
				return wait_for( lock, abs_time - now );
			}
		}

		template <class Clock, class Duration, class Predicate>
		bool wait_until( std::unique_lock<std::mutex>& lock,
						 const std::chrono::time_point<Clock, Duration>& abs_time,
						 Predicate pred )
		{
			while ( !pred() )
			{
				if ( wait_until( lock, abs_time ) == std::cv_status::timeout )
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
