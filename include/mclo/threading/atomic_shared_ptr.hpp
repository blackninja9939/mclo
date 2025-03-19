#pragma once

#include "mclo/memory/tagged_ptr.hpp"

#include <atomic>
#include <compare>
#include <new>

namespace mclo
{
	template <typename T>
	class atomic_shared_ptr;

	namespace detail
	{
		template <typename T>
		class shared_ptr_control_block
		{
		public:
			constexpr shared_ptr_control_block( T* ptr ) noexcept
				: ptr( ptr )
			{
			}

			void add_ref( std::size_t count = 1 ) noexcept
			{
				ref_count.fetch_add( count, std::memory_order_relaxed );
			}

			void release_ref( std::size_t count = 1 ) noexcept
			{
				if ( ref_count.fetch_sub( count, std::memory_order_acq_rel ) == 1 )
				{
					delete_object();
					release_weak();
				}
			}

			void add_weak( std::size_t count = 1 ) noexcept
			{
				weak_count.fetch_add( count, std::memory_order_relaxed );
			}

			void release_weak( std::size_t count = 1 ) noexcept
			{
				if ( weak_count.fetch_sub( count, std::memory_order_acq_rel ) == 1 )
				{
					delete_this();
				}
			}

			[[nodiscard]] std::size_t use_count() const noexcept
			{
				return ref_count.load( std::memory_order_relaxed );
			}

			[[nodiscard]] T* get() const noexcept
			{
				return ptr;
			}

		private:
			void delete_this() noexcept
			{
				delete this;
			}

			void delete_object() noexcept
			{
				delete ptr;
			}

			T* ptr;
			std::atomic_size_t ref_count{ 1 };
			std::atomic_size_t weak_count{ 1 };
		};

		// This is a simplified version of std::shared_ptr for use with atomic_shared_ptr
		// It does not support custom deleters or allocators or aliasing constructors

		template <typename T>
		class shared_ptr
		{
		public:
			friend class atomic_shared_ptr<T>;
			using element_type = std::remove_extent_t<T>;
			// using weak_type = weak_ptr<T>;

			constexpr shared_ptr() noexcept = default;
			~shared_ptr() noexcept
			{
				if ( m_control_block )
				{
					m_control_block->release_ref();
				}
			}

			constexpr shared_ptr( std::nullptr_t ) noexcept {};
			shared_ptr& operator=( std::nullptr_t ) noexcept
			{
				reset();
				return *this;
			}

			explicit shared_ptr( T* ptr )
				: m_control_block( new shared_ptr_control_block<T>( ptr ) )
			{
			}

			shared_ptr( const shared_ptr& other ) noexcept
				: m_control_block( other.m_control_block )
			{
				if ( m_control_block )
				{
					m_control_block->add_ref();
				}
			}
			shared_ptr& operator=( const shared_ptr& other ) noexcept
			{
				reset();
				m_control_block = other.m_control_block;
				if ( m_control_block )
				{
					m_control_block->add_ref();
				}
				return *this;
			}

			shared_ptr( shared_ptr&& other ) noexcept
				: m_control_block( std::exchange( other.m_control_block, nullptr ) )
			{
			}
			shared_ptr& operator=( shared_ptr&& other ) noexcept
			{
				reset();
				m_control_block = std::exchange( other.m_control_block, nullptr );
				return *this;
			}

			void reset() noexcept
			{
				shared_ptr().swap( *this );
			}
			void reset( T* ptr )
			{
				shared_ptr( ptr ).swap( *this );
			}

			void swap( shared_ptr& other ) noexcept
			{
				std::swap( m_control_block, other.m_control_block );
			}

			[[nodiscard]] element_type* get() const noexcept
			{
				return m_control_block ? m_control_block->get() : nullptr;
			}

			[[nodiscard]] T& operator*() const noexcept
				requires( !std::is_void_v<T> )
			{
				return *get();
			}

			[[nodiscard]] T* operator->() const noexcept
				requires( !std::is_array_v<T> )
			{
				return get();
			}

			[[nodiscard]] element_type& operator[]( std::ptrdiff_t idx ) const noexcept
				requires( std::is_array_v<T> )
			{
				return get()[ idx ];
			}

			[[nodiscard]] explicit operator bool() const noexcept
			{
				return get() != nullptr;
			}

			[[nodiscard]] std::size_t use_count() const noexcept
			{
				return m_control_block ? m_control_block->use_count() : 0;
			}

			[[nodiscard]] friend bool operator==( const shared_ptr& lhs, const shared_ptr& rhs ) noexcept
			{
				return lhs.get() == rhs.get();
			}
			[[nodiscard]] friend bool operator<=>( const shared_ptr& lhs, const shared_ptr& rhs ) noexcept
			{
				return std::compare_three_way{}( lhs.get(), rhs.get() );
			}

			[[nodiscard]] friend bool operator==( const shared_ptr& lhs, const std::nullptr_t ) noexcept
			{
				return !lhs;
			}
			[[nodiscard]] friend bool operator<=>( const shared_ptr& lhs, const std::nullptr_t ) noexcept
			{
				return std::compare_three_way{}( lhs.get(), static_cast<element_type*>( nullptr ) );
			}

		private:
			explicit shared_ptr( shared_ptr_control_block<T>* const control_block ) noexcept
				: m_control_block( control_block )
			{
			}

			shared_ptr_control_block<T>* m_control_block = nullptr;
		};
	}

	template <typename T>
	class atomic_shared_ptr
	{
	public:
		using shared_ptr = detail::shared_ptr<T>;

	private:
		using control_block_type = detail::shared_ptr_control_block<T>;
		using local_count_type = std::uint16_t;
		using tag_ptr = mclo::tagged_ptr<control_block_type, local_count_type>;
		using packed_type = typename tag_ptr::packed_type;

		static_assert( tag_ptr::can_store_tag( std::numeric_limits<local_count_type>::max() ) );
		static_assert( std::atomic<packed_type>::is_always_lock_free );

		class local_access
		{
		public:
			// Increments the local ref count
			local_access( std::atomic<packed_type>& packed, const std::memory_order order ) noexcept
				: m_packed( packed )
				, m_local( m_packed.fetch_add( 1, order ) )
			{
			}

			~local_access()
			{
				release( std::memory_order_acq_rel );
			}

			void refresh( tag_ptr new_value, const std::memory_order order ) noexcept
			{
				if ( new_value.get() == m_local.get() )
				{
					return;
				}
				release();

				packed_type expected = m_local.packed();
				packed_type desired = new_value.packed();
				while ( true)
				{
					++desired;
					if ( m_packed.compare_exchange_weak( expected, desired, order ) )
					{
						break;
					}
				}
				m_local = tag_ptr( desired );
			}

			[[nodiscard]] const tag_ptr& get() const noexcept
			{
				return m_local.get();
			}

			[[nodiscard]] shared_ptr get_shared() noexcept
			{
				if ( !m_local )
				{
					return {};
				}
				m_local->add_ref();
				return shared_ptr( m_local.get() );
			}

		private:
			void release( const std::memory_order order ) noexcept
			{
				control_block_type* const control_block = m_local.get();

				// fetch_add added one but returns old, so account for addition here
				packed_type expected = m_local.packed() + 1;
				while ( true )
				{
					// On success means we decremented our local ref aka we pushed it to the main one
					if ( m_packed.compare_exchange_weak( expected, expected - 1, order ) )
					{
						break;
					}

					// Control block changed, decrement the old one and this object is now not our responsibility
					const tag_ptr expected_ptr{ expected };
					if ( expected_ptr != control_block )
					{
						control_block->release_ref();
						break;
					}

					// We failed but its the same pointer, so try to add our ref count in again
				}
			}

			std::atomic<packed_type>& m_packed;
			tag_ptr m_local;
		};

	public:
		constexpr atomic_shared_ptr() noexcept = default;

		constexpr ~atomic_shared_ptr()
		{
			tag_ptr local_ptr( m_packed.load( std::memory_order_relaxed ) );
			local_ptr->release_ref();
		}

		constexpr atomic_shared_ptr( const std::nullptr_t ) noexcept {};
		void operator=( const std::nullptr_t ) noexcept
		{
			store( nullptr );
		}

		atomic_shared_ptr( shared_ptr desired ) noexcept
		{
			store( std::move( desired ), std::memory_order_relaxed );
		}

		void operator=( shared_ptr desired ) noexcept
		{
			store( std::move( desired ) );
		}

		atomic_shared_ptr( const atomic_shared_ptr& other ) = delete;
		atomic_shared_ptr& operator=( const atomic_shared_ptr& other ) = delete;

		static constexpr bool is_always_lock_free = true;

		[[nodiscard]] bool is_lock_free() const noexcept
		{
			return true;
		}

		void store( shared_ptr desired, const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			DEBUG_ASSERT( is_valid_store_order( order ), "Invalid store memory order" );
			// We drop the return value which releases the reference we held to the old control block
			exchange( std::move( desired ), order );
		}

		[[nodiscard]] shared_ptr load( const std::memory_order order = std::memory_order_seq_cst ) const noexcept
		{
			DEBUG_ASSERT( is_valid_load_order( order ), "Invalid load memory order" );
			local_access local( m_packed, order );
			return local.get_shared();
		}

		[[nodiscard]] operator shared_ptr() const noexcept
		{
			return load();
		}

		shared_ptr exchange( shared_ptr desired, const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			if ( desired.m_control_block )
			{
				desired.m_control_block->add_ref();
			}

			const packed_type desired_packed = tag_ptr( desired.m_control_block ).packed();
			tag_ptr old( m_packed.exchange( desired_packed, order ) );

			if ( old )
			{
				old->add_ref( old.tag() );
			}

			// Adopts ownership so we do not need to release the old control block
			return shared_ptr( old.get() );
		}

		bool compare_exchange_weak( shared_ptr& expected,
									shared_ptr desired,
									const std::memory_order success,
									const std::memory_order failure ) noexcept
		{
			local_access local( m_packed, std::memory_order_relaxed );
			if ( local.get() != expected.m_control_block )
			{
				expected = local.get_shared();
				return false;
			}

			tag_ptr desired_ptr = tag_ptr( desired.m_control_block );
			if ( local.get().get() == desired_ptr.get() )
			{
				return true;
			}

			tag_ptr old_ptr = local.get();
			packed_type old_packed = old_ptr.packed();
			const packed_type desired_packed = desired_ptr.packed();

			if ( m_packed.compare_exchange_weak( old_packed, desired_packed, success, failure ) )
			{
				if ( old_ptr )
				{
					old_ptr->add_ref( old_ptr.tag() );
					old_ptr->release_ref();
				}
				return true;
			}
			else
			{
				local.refresh( old_packed, failure );
				expected = local.get_shared();
				return false;
			}
		}

		bool compare_exchange_strong( shared_ptr& expected,
									  shared_ptr desired,
									  const std::memory_order success,
									  const std::memory_order failure ) noexcept
		{
			const shared_ptr local_expected = expected;
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

		bool compare_exchange_weak( shared_ptr& expected,
									shared_ptr desired,
									const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			return compare_exchange_weak( expected, std::move( desired ), order, map_compare_fail_order( order ) );
		}

		bool compare_exchange_strong( shared_ptr& expected,
									  shared_ptr desired,
									  const std::memory_order order = std::memory_order_seq_cst ) noexcept
		{
			return compare_exchange_strong( expected, std::move( desired ), order, map_compare_fail_order( order ) );
		}

		//void wait( shared_ptr old, const std::memory_order order = std::memory_order_seq_cst ) const noexcept;

		//void notify_one() noexcept;
		//void notify_all() noexcept;

	private:
		[[nodiscard]] static constexpr std::memory_order map_compare_fail_order(
			const std::memory_order order ) noexcept
		{
			switch ( order )
			{
				case std::memory_order_acq_rel:
					return std::memory_order_acquire;
				case std::memory_order_release:
					return std::memory_order_relaxed;
				default:
					return order;
			}
		}

		[[nodiscard]] static constexpr bool is_valid_load_order( const std::memory_order order ) noexcept
		{
			switch ( order )
			{
				case std::memory_order_release:
				case std::memory_order_acq_rel:
					return false;
				default:
					return true;
			}
		}

		[[nodiscard]] static constexpr bool is_valid_store_order( const std::memory_order order ) noexcept
		{
			switch ( order )
			{
				case std::memory_order_consume:
				case std::memory_order_acquire:
				case std::memory_order_acq_rel:
					return false;
				default:
					return true;
			}
		}

		mutable std::atomic<packed_type> m_packed{ 0 };
	};
}
