#pragma once

#include "mclo/memory/intrusive_ptr.hpp"
#include "mclo/threading/atomic128.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace mclo
{
	/// @brief A lock-free atomic owning smart pointer, the @c atomic_shared_ptr counterpart of @c std::atomic over a
	/// @c shared_ptr.
	/// @details Implements the split reference counting scheme described by Anthony Williams (N4162): the slot packs a
	/// pointer with a per-slot access count into a 16 byte value updated with a double width compare exchange
	/// (@c atomic128), while the object's strong count and external counter live in @c atomic_shared_ptr_ref_counter.
	/// Loading a
	/// value safely reserves the object via the access count before taking a strong reference, so concurrent loads and
	/// stores never tear or use-after-free without any lock.
	/// @tparam T An intrusively reference counted object type. It must satisfy the same customization points as
	/// @c intrusive_ptr (@c intrusive_ptr_add_ref and @c intrusive_ptr_release_ref) plus the slot reconciliation
	/// points @c atomic_shared_ptr_ref_counter_add_external and @c atomic_shared_ptr_ref_counter_remove_external, all
	/// found by ADL. Deriving from @c atomic_shared_ptr_ref_counter<T> provides all of these, but any type honouring
	/// the contract works.
	/// @note Operations on the @c atomic_shared_ptr itself are atomic; the returned @c intrusive_ptr values are
	/// ordinary non-atomic strong owners.
	template <typename T>
	class atomic_shared_ptr
	{
		struct counted_ptr
		{
			T* m_ptr;
			std::int64_t m_access_count;
		};

		// RAII guard that reserves the current slot value so it cannot be reclaimed while we take a strong reference.
		struct local_access
		{
			atomic128<counted_ptr>& m_slot;
			counted_ptr m_val;

			local_access( atomic128<counted_ptr>& slot,
						  const std::memory_order order = std::memory_order_relaxed ) noexcept
				: m_slot( slot )
				, m_val( slot.load( order ) )
			{
				acquire( order );
			}

			local_access( const local_access& ) = delete;
			local_access& operator=( const local_access& ) = delete;

			~local_access()
			{
				release();
			}

			void acquire( const std::memory_order order ) noexcept
			{
				if ( !m_val.m_ptr )
				{
					return;
				}
				counted_ptr desired;
				do
				{
					desired = m_val;
					++desired.m_access_count;
				}
				while ( !m_slot.compare_exchange_weak( m_val, desired, order, std::memory_order_relaxed ) );
				m_val = desired;
			}

			void release() noexcept
			{
				if ( !m_val.m_ptr )
				{
					return;
				}
				counted_ptr target = m_val;
				counted_ptr desired;
				do
				{
					desired = target;
					--desired.m_access_count;
					if ( m_slot.compare_exchange_weak( target, desired ) )
					{
						return;
					}
				}
				while ( target.m_ptr == m_val.m_ptr );

				// The slot value changed under us, so whoever replaced it took ownership of our reservation and we
				// hand it back through the object's external counter instead.
				atomic_shared_ptr_ref_counter_remove_external( m_val.m_ptr );
			}

			void refresh( const counted_ptr newval, const std::memory_order order ) noexcept
			{
				if ( newval.m_ptr == m_val.m_ptr )
				{
					return;
				}
				release();
				m_val = newval;
				acquire( order );
			}

			[[nodiscard]] T* get_ptr() const noexcept
			{
				return m_val.m_ptr;
			}

			[[nodiscard]] intrusive_ptr<T> get_shared_ptr() const noexcept
			{
				return intrusive_ptr<T>( m_val.m_ptr );
			}
		};

	public:
		/// @brief The pointed-to object type.
		using element_type = T;

		/// @brief @c true if operations are always lock-free on this platform, mirroring the underlying @c atomic128.
		static constexpr bool is_always_lock_free = atomic128<counted_ptr>::is_always_lock_free;

		/// @brief Constructs an empty atomic pointer holding @c nullptr.
		atomic_shared_ptr() noexcept = default;

		/// @brief Constructs an empty atomic pointer holding @c nullptr.
		atomic_shared_ptr( std::nullptr_t ) noexcept
		{
		}

		/// @brief Constructs the atomic pointer taking ownership of @p desired.
		/// @param desired The strong reference to adopt; its ownership is transferred into the slot.
		atomic_shared_ptr( intrusive_ptr<T> desired ) noexcept
			: m_slot( counted_ptr{ desired.detatch(), 0 } )
		{
		}

		atomic_shared_ptr( const atomic_shared_ptr& ) = delete;
		atomic_shared_ptr& operator=( const atomic_shared_ptr& ) = delete;

		~atomic_shared_ptr()
		{
			// No concurrent access is permitted during destruction so the access count must already be zero.
			const counted_ptr old = m_slot.load( std::memory_order_relaxed );
			if ( old.m_ptr )
			{
				intrusive_ptr_release_ref( old.m_ptr );
			}
		}

		/// @brief Checks whether operations on this atomic pointer are lock-free.
		/// @return @c true if the underlying @c atomic128 is lock-free.
		[[nodiscard]] bool is_lock_free() const noexcept
		{
			return m_slot.is_lock_free();
		}

		/// @brief Atomically loads a strong reference to the currently held object.
		/// @param order The memory order for the load.
		/// @return A strong reference to the held object, or an empty @c intrusive_ptr if none is held.
		[[nodiscard]] intrusive_ptr<T> load( const std::memory_order order = std::memory_order_seq_cst ) const noexcept
		{
			const local_access guard( m_slot, order );
			return guard.get_shared_ptr();
		}

		/// @brief Implicitly loads a strong reference to the currently held object.
		/// @return A strong reference to the held object, or an empty @c intrusive_ptr if none is held.
		operator intrusive_ptr<T>() const noexcept
		{
			return load();
		}

		/// @brief Atomically replaces the held object with @p desired, releasing the previous one.
		/// @param desired The strong reference to store; its ownership is transferred into the slot.
		/// @param order The memory order for the store.
		void store( intrusive_ptr<T> desired, const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			const counted_ptr newval{ desired.get(), 0 };
			const counted_ptr old = m_slot.exchange( newval, order );
			if ( old.m_ptr )
			{
				atomic_shared_ptr_ref_counter_add_external( old.m_ptr, old.m_access_count );
				intrusive_ptr_release_ref( old.m_ptr );
			}
			desired.detatch();
		}

		/// @brief Atomically replaces the held object with @p desired, releasing the previous one.
		/// @param desired The strong reference to store.
		/// @return A reference to this atomic pointer.
		atomic_shared_ptr& operator=( intrusive_ptr<T> desired ) noexcept
		{
			store( std::move( desired ) );
			return *this;
		}

		/// @brief Atomically clears the held object, releasing the previous one.
		/// @return A reference to this atomic pointer.
		atomic_shared_ptr& operator=( std::nullptr_t ) noexcept
		{
			store( intrusive_ptr<T>{} );
			return *this;
		}

		/// @brief Atomically replaces the held object with @p desired and returns the previous one.
		/// @param desired The strong reference to store; its ownership is transferred into the slot.
		/// @param order The memory order for the read-modify-write operation.
		/// @return The strong reference previously held, or an empty @c intrusive_ptr if none was held.
		[[nodiscard]] intrusive_ptr<T> exchange( intrusive_ptr<T> desired,
												 const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			const counted_ptr newval{ desired.get(), 0 };
			const counted_ptr old = m_slot.exchange( newval, order );
			intrusive_ptr<T> result( old.m_ptr );
			if ( old.m_ptr )
			{
				atomic_shared_ptr_ref_counter_add_external( old.m_ptr, old.m_access_count );
				intrusive_ptr_release_ref( old.m_ptr );
			}
			desired.detatch();
			return result;
		}

		/// @brief Atomically compares the held object with @p expected and, if they match, replaces it with @p desired.
		/// @details May fail spuriously even when the held object equals @p expected, so it should be called in a loop.
		/// @param expected The strong reference expected to be held; on failure it is updated to the actual held
		/// object.
		/// @param desired The strong reference to store on success; its ownership is transferred on success.
		/// @param success The memory order to use if the comparison succeeds.
		/// @param failure The memory order to use if the comparison fails.
		/// @return @c true if the exchange took place, @c false otherwise.
		bool compare_exchange_weak( intrusive_ptr<T>& expected,
									intrusive_ptr<T> desired,
									const std::memory_order success = std::memory_order_seq_cst,
									const std::memory_order failure = std::memory_order_seq_cst ) noexcept
		{
			local_access guard( m_slot, failure );
			if ( guard.get_ptr() != expected.get() )
			{
				expected = guard.get_shared_ptr();
				return false;
			}

			counted_ptr oldval = guard.m_val;
			const counted_ptr newval{ desired.get(), 0 };
			if ( oldval.m_ptr == newval.m_ptr )
			{
				return true;
			}

			if ( m_slot.compare_exchange_weak( oldval, newval, success, failure ) )
			{
				if ( oldval.m_ptr )
				{
					atomic_shared_ptr_ref_counter_add_external( oldval.m_ptr, oldval.m_access_count );
					intrusive_ptr_release_ref( oldval.m_ptr );
				}
				desired.detatch();
				return true;
			}

			guard.refresh( oldval, failure );
			expected = guard.get_shared_ptr();
			return false;
		}

		/// @brief Atomically compares the held object with @p expected and, if they match, replaces it with @p desired.
		/// @details Does not fail spuriously, retrying internally until the held object differs from @p expected.
		/// @param expected The strong reference expected to be held; on failure it is updated to the actual held
		/// object.
		/// @param desired The strong reference to store on success; its ownership is transferred on success.
		/// @param success The memory order to use if the comparison succeeds.
		/// @param failure The memory order to use if the comparison fails.
		/// @return @c true if the exchange took place, @c false otherwise.
		bool compare_exchange_strong( intrusive_ptr<T>& expected,
									  intrusive_ptr<T> desired,
									  const std::memory_order success = std::memory_order_seq_cst,
									  const std::memory_order failure = std::memory_order_seq_cst ) noexcept
		{
			const intrusive_ptr<T> local_expected = expected;
			do
			{
				if ( compare_exchange_weak( expected, desired, success, failure ) )
				{
					return true;
				}
			}
			while ( expected == local_expected );
			return false;
		}

	private:
		mutable atomic128<counted_ptr> m_slot{
			counted_ptr{ nullptr, 0 }
        };
	};
}
