#pragma once

#include <atomic>

namespace mclo
{
	/// @brief CRTP base providing an atomic reference count for use with @ref intrusive_ptr.
	/// @details Derive from @c intrusive_ref_counter<Derived> to make @p Derived usable with @ref intrusive_ptr. The
	/// count is thread-safe and, when it reaches zero, the object deletes itself as a @p Derived. Copying a counted
	/// object does not copy the count: a fresh copy starts with its own zero count, since reference counts track
	/// ownership of a particular object, not its value.
	/// @tparam Derived The derived type, deleted via @c delete when the count reaches zero.
	template <typename Derived>
	class intrusive_ref_counter
	{
	public:
		intrusive_ref_counter() noexcept = default;

		/// @brief Copy constructor. Starts the new object's count at zero rather than copying @p other's.
		intrusive_ref_counter( const intrusive_ref_counter& ) noexcept
		{
		}

		/// @brief Copy assignment. Leaves this object's count unchanged.
		intrusive_ref_counter& operator=( const intrusive_ref_counter& ) noexcept
		{
			return *this;
		}

		/// @brief Returns the current reference count.
		/// @note In concurrent code the value is a snapshot and may be stale once read.
		std::size_t use_count() const noexcept
		{
			return m_counter.load( std::memory_order_relaxed );
		}

		/// @brief Increments the reference count of @p ptr. Used by @ref intrusive_ptr.
		friend void intrusive_ptr_add_ref( const intrusive_ref_counter<Derived>* ptr ) noexcept
		{
			ptr->m_counter.fetch_add( 1, std::memory_order_relaxed );
		}

		/// @brief Decrements the reference count of @p ptr, deleting it when the count reaches zero.
		/// @details Used by @ref intrusive_ptr. Deletes @p ptr as a @p Derived once the last reference is released.
		friend void intrusive_ptr_release_ref( const intrusive_ref_counter<Derived>* ptr ) noexcept
		{
			const std::size_t old = ptr->m_counter.fetch_sub( 1, std::memory_order_release );
			MCLO_DEBUG_ASSERT( old != 0, "Reference count underflow in intrusive_ref_counter" );
			if ( old == 1 ) // Was last reference
			{
				std::atomic_thread_fence( std::memory_order_acquire );
				delete static_cast<const Derived*>( ptr );
			}
		}

	protected:
		~intrusive_ref_counter() = default;

	private:
		mutable std::atomic_size_t m_counter{ 0 };
	};
}
