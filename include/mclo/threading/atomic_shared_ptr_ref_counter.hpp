#pragma once

#include "mclo/debug/assert.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace mclo
{
	/// @brief Intrusive reference counter base allowing an object to be owned by @c intrusive_ptr and stored in a
	/// lock-free @c atomic_shared_ptr.
	/// @details Uses a split reference count: the normal strong @c count is packed alongside a count of
	/// @c external_counters that track threads concurrently accessing the object through an @c atomic_shared_ptr slot.
	/// Packing both into a single atomic word is required so the two distinct deletion conditions can be evaluated
	/// atomically. Objects opt in by deriving from @c atomic_shared_ptr_ref_counter with themselves as @p Derived
	/// (CRTP). Unlike @c intrusive_ref_counter this counter carries the extra external counter machinery required to
	/// be reclaimed safely from a lock-free @c atomic_shared_ptr.
	/// @tparam Derived The most derived type, deleted via @c static_cast when the last reference is released.
	/// @warning @p Derived must publicly derive from @c atomic_shared_ptr_ref_counter<Derived> and must not be deleted
	/// directly while any @c intrusive_ptr or @c atomic_shared_ptr still references it.
	template <typename Derived>
	class atomic_shared_ptr_ref_counter
	{
	public:
		/// @brief Constructs the counter with zero references.
		atomic_shared_ptr_ref_counter() noexcept = default;

		/// @brief Copy constructs a fresh counter, the reference counts are not copied.
		/// @details A new object always starts with its own independent reference count regardless of the source.
		atomic_shared_ptr_ref_counter( const atomic_shared_ptr_ref_counter& ) noexcept
		{
		}

		/// @brief Copy assignment is a no-op, the reference counts of neither object are changed.
		atomic_shared_ptr_ref_counter& operator=( const atomic_shared_ptr_ref_counter& ) noexcept
		{
			return *this;
		}

		/// @brief An approximate count of strong owners, intended for diagnostics and testing only.
		/// @return The number of strong references plus one if any external counters are outstanding.
		/// @warning The value may be stale the instant it is returned in the presence of concurrent owners.
		[[nodiscard]] std::size_t use_count() const noexcept
		{
			const counter current = m_counter.load( std::memory_order_relaxed );
			return static_cast<std::size_t>( current.m_count ) + ( current.m_external_counters != 0 ? 1u : 0u );
		}

		/// @brief Increments the strong reference count, the @c intrusive_ptr add-reference customization point.
		/// @param ptr The object to add a reference to.
		friend void intrusive_ptr_add_ref( const atomic_shared_ptr_ref_counter<Derived>* const ptr ) noexcept
		{
			ptr->inc_count();
		}

		/// @brief Decrements the strong reference count and deletes @p ptr if it was the last owner, the
		/// @c intrusive_ptr release customization point.
		/// @param ptr The object to release a reference from.
		friend void intrusive_ptr_release_ref( const atomic_shared_ptr_ref_counter<Derived>* const ptr ) noexcept
		{
			ptr->dec_count();
		}

		/// @brief Folds a slot's accumulated access count back into the object's external counter.
		/// @details Used by @c atomic_shared_ptr when a slot is replaced or destroyed to reconcile the per-slot
		/// access count into the object. @p amount is a signed displacement and is applied modulo 2^32.
		/// @param ptr The object whose external counter is updated.
		/// @param amount The signed displacement to add to the external counter.
		friend void atomic_shared_ptr_ref_counter_add_external( const atomic_shared_ptr_ref_counter<Derived>* const ptr,
																const std::int64_t amount ) noexcept
		{
			ptr->add_external_counters( amount );
		}

		/// @brief Removes a single external counter, deleting @p ptr if it was the last outstanding reference.
		/// @details Used by @c atomic_shared_ptr when a reservation outlives the slot it was taken against.
		/// @param ptr The object whose external counter is decremented.
		friend void atomic_shared_ptr_ref_counter_remove_external(
			const atomic_shared_ptr_ref_counter<Derived>* const ptr ) noexcept
		{
			ptr->remove_external_counter();
		}

	protected:
		~atomic_shared_ptr_ref_counter() = default;

	private:
		struct counter
		{
			std::int32_t m_count;
			std::uint32_t m_external_counters;
		};

		void inc_count() const noexcept
		{
			counter old = m_counter.load( std::memory_order_relaxed );
			counter desired;
			do
			{
				desired = old;
				++desired.m_count;
			}
			while ( !m_counter.compare_exchange_weak( old, desired ) );
		}

		void dec_count() const noexcept
		{
			counter old = m_counter.load( std::memory_order_relaxed );
			counter desired;
			do
			{
				desired = old;
				--desired.m_count;
			}
			while ( !m_counter.compare_exchange_weak( old, desired ) );

			MCLO_DEBUG_ASSERT( old.m_count != 0, "Reference count underflow in atomic_shared_ptr_ref_counter" );
			if ( old.m_count == 1 && old.m_external_counters == 0 )
			{
				std::atomic_thread_fence( std::memory_order_acquire );
				delete static_cast<const Derived*>( this );
			}
		}

		void add_external_counters( const std::int64_t amount ) const noexcept
		{
			if ( amount == 0 )
			{
				return;
			}
			counter old = m_counter.load( std::memory_order_relaxed );
			counter desired;
			do
			{
				desired = old;
				// The amount is a signed displacement: a slot's access count may legitimately be negative when a
				// loader released against a different incarnation of the same pointer (ABA). Narrowing to uint32 and
				// letting it wrap is deliberate, the global invariant only needs to hold modulo 2^32 so this exactly
				// cancels the matching over- or under-decrement when the slot is folded back in. Do not "fix" this to
				// a wider type or a checked add.
				desired.m_external_counters += static_cast<std::uint32_t>( amount );
			}
			while ( !m_counter.compare_exchange_weak( old, desired ) );
		}

		void remove_external_counter() const noexcept
		{
			counter old = m_counter.load( std::memory_order_relaxed );
			counter desired;
			do
			{
				desired = old;
				--desired.m_external_counters;
			}
			while ( !m_counter.compare_exchange_weak( old, desired ) );

			if ( old.m_count == 0 && old.m_external_counters == 1 )
			{
				std::atomic_thread_fence( std::memory_order_acquire );
				delete static_cast<const Derived*>( this );
			}
		}

		mutable std::atomic<counter> m_counter{
			counter{ 0, 0 }
        };
	};
}
