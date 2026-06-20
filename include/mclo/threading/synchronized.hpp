#pragma once

#include "mclo/threading/mutex.hpp"

#include <concepts>
#include <functional>
#include <shared_mutex>
#include <type_traits>

namespace mclo
{
	/// @brief A wrapper that couples a value with the mutex guarding it, enforcing locked access.
	/// @details Rather than exposing a value and a separate mutex that callers must remember to lock,
	/// @c synchronized stores both together and only grants access to the value while a lock is held.
	/// Access is performed by passing a callable to @ref with_lock (exclusive) or @ref with_shared_lock
	/// (shared); the value is provided to the callable by reference for the duration of the lock.
	/// @details If @p Mutex models a shared mutex (provides @c lock_shared / @c unlock_shared) then
	/// @ref with_shared_lock and the const read helpers acquire a shared lock, otherwise they fall back
	/// to an exclusive lock.
	/// @code
	/// mclo::synchronized<std::vector<int>> values;
	/// values.with_lock( []( std::vector<int>& v ) { v.push_back( 42 ); } );
	/// const std::size_t size = values.with_shared_lock( []( const std::vector<int>& v ) { return v.size(); } );
	/// @endcode
	/// @tparam T The type of the guarded value.
	/// @tparam Mutex The mutex type guarding the value. Defaults to @c std::shared_mutex to allow
	/// concurrent shared reads.
	template <typename T, typename Mutex = std::shared_mutex>
	class synchronized
	{
	private:
		static constexpr bool is_shared_mutex = requires( Mutex& m ) {
			m.lock_shared();
			m.unlock_shared();
		};

		using lock_type = std::scoped_lock<Mutex>;
		using shared_lock_type = std::conditional_t<is_shared_mutex, std::shared_lock<Mutex>, lock_type>;
		using unique_lock_type = std::unique_lock<Mutex>;

	public:
		/// @brief The type of the guarded value.
		using value_type = T;

		/// @brief The type of the mutex guarding the value.
		using mutex_type = Mutex;

		/// @brief Default constructs the guarded value.
		constexpr synchronized() noexcept( std::is_nothrow_default_constructible_v<value_type> ) = default;

		/// @brief Copy constructs the guarded value.
		/// @param value The value to copy into the guarded storage.
		constexpr explicit synchronized( const value_type& value ) noexcept(
			std::is_nothrow_copy_constructible_v<value_type> )
			: m_data( value )
		{
		}

		/// @brief Move constructs the guarded value.
		/// @param value The value to move into the guarded storage.
		constexpr explicit synchronized( value_type&& value ) noexcept(
			std::is_nothrow_move_constructible_v<value_type> )
			: m_data( std::move( value ) )
		{
		}

		/// @brief Constructs the guarded value in place from the given arguments.
		/// @tparam Args The argument types forwarded to the @p value_type constructor.
		/// @param args The arguments forwarded to the @p value_type constructor.
		template <typename... Args>
		constexpr explicit synchronized( std::in_place_t, Args&&... args ) noexcept(
			std::is_nothrow_constructible_v<value_type, Args...> )
			: m_data( std::forward<Args>( args )... )
		{
		}

		/// @brief Copy assigns the guarded value under an exclusive lock.
		/// @param value The value to copy assign into the guarded storage.
		/// @return A reference to this object.
		synchronized& operator=( const value_type& value )
		{
			if ( &m_data != &value ) [[likely]]
			{
				const lock_type lock( m_mutex );
				m_data = value;
			}
			return *this;
		}

		/// @brief Move assigns the guarded value under an exclusive lock.
		/// @param value The value to move assign into the guarded storage.
		/// @return A reference to this object.
		synchronized& operator=( value_type&& value )
		{
			if ( &m_data != &value ) [[likely]]
			{
				const lock_type lock( m_mutex );
				m_data = std::move( value );
			}
			return *this;
		}

		/// @brief Swaps the guarded values of two @p synchronized objects.
		/// @details Both mutexes are locked together using a deadlock-avoiding algorithm.
		/// @param other The object to swap guarded values with.
		void swap( synchronized& other )
		{
			if ( this != &other ) [[likely]]
			{
				const std::scoped_lock lock( m_mutex, other.m_mutex );
				using std::swap;
				swap( m_data, other.m_data );
			}
		}

		/// @brief Swaps the guarded values of two @p synchronized objects.
		/// @param lhs The first object to swap.
		/// @param rhs The second object to swap.
		friend void swap( synchronized& lhs, synchronized& rhs )
		{
			lhs.swap( rhs );
		}

		/// @brief Invokes a callable with exclusive access to the guarded value.
		/// @details An exclusive lock is held for the duration of the call. @p func is invoked via
		/// @c std::invoke, so a pointer to a member function or member data of the guarded value may be
		/// passed in addition to a regular callable.
		/// @tparam Func The callable invoked with a mutable reference to the guarded value.
		/// @param func The callable invoked via @c std::invoke with the guarded value.
		/// @return The result of invoking @p func.
		/// @warning The reference passed to @p func must not be retained beyond the call, as it is only
		/// valid while the lock is held.
		template <typename Func>
		auto with_lock( Func&& func )
		{
			const lock_type lock( m_mutex );
			return std::invoke( std::forward<Func>( func ), m_data );
		}

		/// @brief Invokes a callable with exclusive access to both the guarded value and its lock.
		/// @details An exclusive lock is acquired and passed, alongside a mutable reference to the guarded
		/// value, to @p func via @c std::invoke. Exposing the @c std::unique_lock lets the callable wait on
		/// a condition variable that shares the same mutex, where the wait releases and re-acquires the
		/// lock around the guarded value.
		/// @code
		/// mclo::synchronized<std::queue<int>, mclo::mutex> queue;
		/// mclo::condition_variable not_empty;
		/// const int item = queue.with_lock( [ & ]( std::queue<int>& q, std::unique_lock<mclo::mutex>& lock ) {
		///     not_empty.wait( lock, [ & ] { return !q.empty(); } );
		///     const int front = q.front();
		///     q.pop();
		///     return front;
		/// } );
		/// @endcode
		/// @tparam Func The callable invoked with a mutable reference to the guarded value and its lock.
		/// @param func The callable invoked via @c std::invoke with the guarded value and the lock.
		/// @return The result of invoking @p func.
		/// @warning Neither the value reference nor the lock passed to @p func may be retained beyond the
		/// call, as they are only valid while the lock is held.
		/// @warning The guarded value may only be accessed while the lock is held. If the lock is released
		/// (for example by a condition variable's wait) it must be re-acquired before the value is read or
		/// modified again, otherwise mutual exclusion is lost and the access is a data race.
		template <typename Func>
		auto with_lock( Func&& func )
			requires std::invocable<Func, value_type&, unique_lock_type&>
		{
			unique_lock_type lock( m_mutex );
			return std::invoke( std::forward<Func>( func ), m_data, lock );
		}

		/// @brief Invokes a callable with shared (read-only) access to the guarded value.
		/// @details A shared lock is held for the duration of the call, allowing concurrent readers when
		/// @p Mutex models a shared mutex, otherwise an exclusive lock is used. @p func is invoked via
		/// @c std::invoke, so a pointer to a member function or member data of the guarded value may be
		/// passed in addition to a regular callable.
		/// @tparam Func The callable invoked with a const reference to the guarded value.
		/// @param func The callable invoked via @c std::invoke with the guarded value.
		/// @return The result of invoking @p func.
		/// @warning The reference passed to @p func must not be retained beyond the call, as it is only
		/// valid while the lock is held.
		template <typename Func>
		auto with_shared_lock( Func&& func ) const
		{
			const shared_lock_type lock( m_mutex );
			return std::invoke( std::forward<Func>( func ), m_data );
		}

		/// @brief Returns a copy of the guarded value taken under a shared lock.
		/// @return A copy of the guarded value.
		[[nodiscard]] value_type copy() const
		{
			const shared_lock_type lock( m_mutex );
			return m_data;
		}

		/// @brief Copies the guarded value into a destination under a shared lock.
		/// @param dest The destination assigned the current guarded value.
		void copy_into( value_type& dest ) const
		{
			const shared_lock_type lock( m_mutex );
			dest = m_data;
		}

	private:
		value_type m_data;
		mutable mutex_type m_mutex;
	};
}
