#pragma once

#include <chrono>

namespace mclo
{
	class [[nodiscard]] null_mutex
	{
	public:
		constexpr null_mutex() noexcept = default;

		null_mutex( const null_mutex& ) = delete;
		null_mutex& operator=( const null_mutex& ) = delete;

		void lock() noexcept
		{
		}

		[[nodiscard]] bool try_lock() noexcept
		{
			return true;
		}

		template <typename Rep, typename Period>
		[[nodiscard]] bool try_lock_for( const std::chrono::duration<Rep, Period>& ) noexcept
		{
			return true;
		}

		template <typename Clock, typename Duration>
		[[nodiscard]] bool try_lock_until( const std::chrono::time_point<Clock, Duration>& ) noexcept
		{
			return true;
		}

		void unlock() noexcept
		{
		}

		void lock_shared() noexcept
		{
		}

		[[nodiscard]] bool try_lock_shared() noexcept
		{
			return true;
		}

		template <typename Rep, typename Period>
		[[nodiscard]] bool try_lock_shared_for( const std::chrono::duration<Rep, Period>& ) noexcept
		{
			return true;
		}

		template <typename Clock, typename Duration>
		[[nodiscard]] bool try_lock_shared_until( const std::chrono::time_point<Clock, Duration>& ) noexcept
		{
			return true;
		}

		void unlock_shared() noexcept
		{
		}
	};
}
