#pragma once

#include <chrono>

namespace mclo
{
	/// @brief A no-op mutex that satisfies the standard mutex and shared mutex named requirements without locking.
	/// @details All operations are empty and trivially succeed. This is intended as a zero-overhead stand-in for a real
	/// mutex when a generic type is parameterised on a lockable but synchronisation is not actually required, such as
	/// in single-threaded configurations.
	/// @warning Provides no synchronisation whatsoever. Using this where concurrent access actually occurs results in
	/// data races and undefined behaviour.
	class [[nodiscard]] null_mutex
	{
	public:
		constexpr null_mutex() noexcept = default;

		null_mutex( const null_mutex& ) = delete;
		null_mutex& operator=( const null_mutex& ) = delete;

		/// @brief Does nothing.
		void lock() noexcept
		{
		}

		/// @brief Does nothing and always succeeds.
		/// @return Always @c true.
		[[nodiscard]] bool try_lock() noexcept
		{
			return true;
		}

		/// @brief Does nothing and always succeeds.
		/// @return Always @c true.
		template <typename Rep, typename Period>
		[[nodiscard]] bool try_lock_for( const std::chrono::duration<Rep, Period>& ) noexcept
		{
			return true;
		}

		/// @brief Does nothing and always succeeds.
		/// @return Always @c true.
		template <typename Clock, typename Duration>
		[[nodiscard]] bool try_lock_until( const std::chrono::time_point<Clock, Duration>& ) noexcept
		{
			return true;
		}

		/// @brief Does nothing.
		void unlock() noexcept
		{
		}

		/// @brief Does nothing.
		void lock_shared() noexcept
		{
		}

		/// @brief Does nothing and always succeeds.
		/// @return Always @c true.
		[[nodiscard]] bool try_lock_shared() noexcept
		{
			return true;
		}

		/// @brief Does nothing and always succeeds.
		/// @return Always @c true.
		template <typename Rep, typename Period>
		[[nodiscard]] bool try_lock_shared_for( const std::chrono::duration<Rep, Period>& ) noexcept
		{
			return true;
		}

		/// @brief Does nothing and always succeeds.
		/// @return Always @c true.
		template <typename Clock, typename Duration>
		[[nodiscard]] bool try_lock_shared_until( const std::chrono::time_point<Clock, Duration>& ) noexcept
		{
			return true;
		}

		/// @brief Does nothing.
		void unlock_shared() noexcept
		{
		}
	};
}
