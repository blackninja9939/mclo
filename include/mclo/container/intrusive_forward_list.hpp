#pragma once

#include "mclo/container/detail/intrusive_forward_list_iterator.hpp"
#include "mclo/container/intrusive_forward_list_hook.hpp"
#include "mclo/debug/assert.hpp"

#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename T, typename Tag = void>
	class intrusive_forward_list
	{
		using hook_type = intrusive_forward_list_hook<Tag>;
		static_assert( std::derived_from<T, hook_type>, "T must be derived from the intrusive list hook" );

		static_assert( std::is_object_v<T>,
					   "The C++ Standard forbids containers of non-object types "
					   "because of [container.requirements]." );

	public:
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using iterator = intrusive_forward_list_iterator<T, Tag>;
		using const_iterator = intrusive_forward_list_iterator<const T, Tag>;

		intrusive_forward_list() noexcept = default;

		intrusive_forward_list( const intrusive_forward_list& other ) = delete;
		intrusive_forward_list& operator=( const intrusive_forward_list& other ) = delete;

		intrusive_forward_list( intrusive_forward_list&& other ) noexcept
			: m_head( std::exchange( other.m_head, hook_type() ) )
		{
		}
		intrusive_forward_list& operator=( intrusive_forward_list&& other ) noexcept
		{
			if ( this != &other )
			{
				m_head = std::exchange( other.m_head, hook_type() );
			}
			return *this;
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
		intrusive_forward_list( It first, Sentinel last ) MCLO_NOEXCEPT_TESTS
		{
			insert_after( before_begin(), std::move( first ), std::move( last ) );
		}

		template <std::ranges::input_range Range>
		intrusive_forward_list( Range&& range ) MCLO_NOEXCEPT_TESTS
		{
			insert_after( before_begin(), std::forward<Range>( range ) );
		}

		~intrusive_forward_list()
		{
			clear();
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return iterator( head() );
		}
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return const_iterator( head() );
		}
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return const_iterator( head() );
		}

		[[nodiscard]] iterator end() noexcept
		{
			return iterator( nullptr );
		}
		[[nodiscard]] const_iterator end() const noexcept
		{
			return const_iterator( nullptr );
		}
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return const_iterator( nullptr );
		}

		[[nodiscard]] iterator before_begin() noexcept
		{
			return iterator( before_begin_ptr() );
		}
		[[nodiscard]] const_iterator before_begin() const noexcept
		{
			return const_iterator( before_begin_ptr() );
		}
		[[nodiscard]] const_iterator cbefore_begin() const noexcept
		{
			return const_iterator( before_begin_ptr() );
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return head() == nullptr;
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::same_as<reference, std::iter_reference_t<It>> )
		void assign( It first, Sentinel last )
		{
			clear();
			insert_after( before_begin(), std::move( first ), std::move( last ) );
		}

		template <std::ranges::input_range Range>
			requires( std::same_as<reference, std::ranges::range_reference_t<Range>> )
		void assign( Range&& range ) MCLO_NOEXCEPT_TESTS
		{
			return assign( std::ranges::begin( range ), std::ranges::end( range ) );
		}

		void push_front( reference value ) MCLO_NOEXCEPT_TESTS
		{
			insert_after( before_begin(), value );
		}

		pointer pop_front() MCLO_NOEXCEPT_TESTS
		{
			return cast( unwrap_iterator( erase_after( before_begin() ) ) );
		}

		iterator insert_after( const_iterator pos, reference value ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos != end(), "Cannot insert after end of forward list" );
			hook_type* const ptr = unwrap_iterator( pos );
			hook_type* const hook = std::addressof( value );
			hook->m_next = std::exchange( ptr->m_next, hook );
			return iterator( cast( hook ) );
		}

		template <std::input_iterator It, std::sentinel_for<It> Sentinel>
			requires( std::same_as<reference, std::iter_reference_t<It>> )
		iterator insert_after( const_iterator pos, It first, Sentinel last ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos != end(), "Cannot insert after end of forward list" );
			if ( first == last )
			{
				return pos;
			}
			hook_type* ptr = unwrap_iterator( pos );
			hook_type* const original_next = ptr->m_next;
			while ( first != last )
			{
				hook_type* ref = std::to_address( first++ );
				ptr->m_next = ref;
				ptr = ref;
			}
			ptr->m_next = original_next;
			return iterator( cast( ptr ) );
		}

		template <std::ranges::input_range Range>
			requires( std::same_as<reference, std::ranges::range_reference_t<Range>> )
		iterator insert_after( const_iterator pos, Range&& range ) MCLO_NOEXCEPT_TESTS
		{
			return insert_after( pos, std::ranges::begin( range ), std::ranges::end( range ) );
		}

		iterator erase_after( const_iterator pos ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos != end(), "Cannot erase after end of forward list" );
			hook_type* const ptr = unwrap_iterator( pos );
			hook_type* const original_next = ptr->m_next;
			if ( ptr->m_next )
			{
				ptr->m_next = std::exchange( ptr->m_next->m_next, nullptr );
			}
			return iterator( cast( original_next ) );
		}

		iterator erase_after( const_iterator first, const_iterator last ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( first != end(), "Cannot erase after end of forward list" );
			DEBUG_ASSERT( last != end(), "Cannot erase after end of forward list" );
			hook_type* ptr = unwrap_iterator( first );
			hook_type* const last_ptr = unwrap_iterator( last );
			if ( ptr != last_ptr )
			{
				for ( ;; )
				{
					const auto next = ptr->m_next;
					if ( next == last_ptr )
					{
						break;
					}
					ptr->m_next = std::exchange( next->m_next, nullptr );
				}
			}
		}

		void splice_after( const_iterator pos, intrusive_forward_list& other ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( this != &other, "Cannot splice in same container" );
			DEBUG_ASSERT( pos != end(), "Cannot splice after end of forward list" );
			splice_after( pos, other, other.before_begin(), other.end() );
		}

		void splice_after( const_iterator pos, intrusive_forward_list&& other ) MCLO_NOEXCEPT_TESTS
		{
			splice_after( pos, other );
		}

		void splice_after( const_iterator pos, intrusive_forward_list&, const_iterator it ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos != end(), "Cannot splice after end of forward list" );

			hook_type* const insert = unwrap_iterator( pos );
			hook_type* const prev = unwrap_iterator( it );

			if ( insert != prev )
			{
				hook_type* const first = prev->m_next;
				if ( insert != first )
				{
					prev->m_next = first->m_next;
					first->m_next = insert->m_next;
					insert->m_next = first;
				}
			}
		}

		void splice_after( const_iterator pos, intrusive_forward_list&& other, const_iterator it ) MCLO_NOEXCEPT_TESTS
		{
			splice_after( pos, other, it );
		}

		void splice_after( const_iterator pos, intrusive_forward_list&, const_iterator first, const_iterator last )
			MCLO_NOEXCEPT_TESTS
		{
			if ( first == last )
			{
				return;
			}

			const_iterator after = first;
			++after;
			if ( after == last )
			{
				return;
			}

			const_iterator prev_last = first;
			do
			{
				prev_last = after;
				++after;
			}
			while ( after != last );

			hook_type* const unwrapped_first = unwrap_iterator( first );
			hook_type* const unwrapped_after = unwrap_iterator( after );
			hook_type* const unwrapped_prev_last = unwrap_iterator( prev_last );
			hook_type* const unwrapped_pos = unwrap_iterator( pos );

			hook_type* const extracted_head = unwrapped_first->m_next;

			unwrapped_first->m_next = unwrapped_after;
			unwrapped_prev_last->m_next = unwrapped_pos->m_next;
			unwrapped_pos->m_next = extracted_head;
		}

		void splice_after( const_iterator pos,
						   intrusive_forward_list&& other,
						   const_iterator first,
						   const_iterator last ) MCLO_NOEXCEPT_TESTS
		{
			splice_after( pos, other, first, last );
		}

		template <typename U>
		size_type remove( const U& value )
		{
			return remove( [ &value ]( const_reference object ) { return object == value; } );
		}

		template <typename UnaryPredicate>
		size_type remove_if( UnaryPredicate predicate )
		{
			size_type count = 0;
			hook_type* before_head = before_begin_ptr();
			hook_type* head = head();
			while ( head )
			{
				if ( predicate( *cast( head ) ) )
				{
					head = before_head->m_next = std::exchange( head->m_next, nullptr );
					++count;
				}
				else
				{
					before_head = head;
					head = head->m_next;
				}
			}
			return count;
		}

		void reverse() noexcept
		{
			hook_type* current = m_head.m_next;
			if ( !current )
			{
				// empty forward_list
				return;
			}

			hook_type* prev = nullptr;
			for ( ;; )
			{
				hook_type* const next = current->m_next;
				current->m_next = prev;
				if ( !next )
				{
					m_head.m_next = current;
					return;
				}

				prev = current;
				current = next;
			}
		}

		size_type unique()
		{
			return unique( std::equal_to<>{} );
		}

		template <typename BinaryPredicate>
		size_type unique( BinaryPredicate predicate )
		{
			hook_type* head = this->head();
			size_type count = 0;

			if ( head )
			{
				hook_type* next = head->m_next;
				while ( next )
				{
					if ( predicate( *cast( head ), *cast( next ) ) )
					{
						next = head->m_next = std::exchange( next->m_next, nullptr );
						++count;
					}
					else
					{
						head = next;
						next = next->m_next;
					}
				}
			}

			return count;
		}

		void sort()
		{
			sort( std::less<>{} );
		}

		template <typename Compare>
		void sort();

		template <typename Compare>
		void merge( intrusive_forward_list& other, Compare comp )
		{
			const const_iterator last( this->cend() );
			const const_iterator other_last( other.cend() );

			const_iterator bb( this->cbefore_begin() );
			const_iterator bb_next;

			while ( !other.empty() )
			{
				const_iterator ibx_next( other.cbefore_begin() );
				const_iterator ibx( ibx_next++ );
				while ( ++( bb_next = bb ) != last && !comp( *ibx_next, *bb_next ) )
				{
					bb = bb_next;
				}
				if ( bb_next == last )
				{
					// Now transfer the rest to the end of the container
					this->splice_after( bb, other );
					break;
				}
				else
				{
					size_type n = 0;
					do
					{
						ibx = ibx_next;
						++n;
					}
					while ( ++( ibx_next = ibx ) != other_last && comp( *ibx_next, *bb_next ) );
					this->splice_after( bb, other, other.before_begin(), ibx, n );
				}
			}
		}

		template <typename Compare>
		void merge( intrusive_forward_list&& other, Compare comp )
		{
			merge( other, comp );
		}

		void merge( intrusive_forward_list& other )
		{
			merge( other, std::less<>{} );
		}

		void merge( intrusive_forward_list&& other )
		{
			merge( other );
		}

		template <std::invocable<pointer> Func>
			requires( std::is_nothrow_invocable_v<Func, pointer> )
		void consume( Func func ) noexcept
		{
			hook_type* head = std::exchange( m_head.m_next, nullptr );
			while ( head )
			{
				hook_type* const next = std::exchange( head->m_next, nullptr );
				func( cast( head ) );
				head = next;
			}
		}

		void clear() noexcept
		{
			consume( []( pointer ) noexcept {} );
		}

		void swap( intrusive_forward_list& other ) noexcept
		{
			using std::swap;
			swap( m_head, other.m_head );
		}

		friend void swap( intrusive_forward_list& lhs, intrusive_forward_list& rhs ) noexcept
		{
			lhs.swap( rhs );
		}

	private:
		[[nodiscard]] static pointer cast( hook_type* ptr ) noexcept
		{
			return static_cast<pointer>( ptr );
		}
		[[nodiscard]] static const_pointer cast( const hook_type* ptr ) noexcept
		{
			return static_cast<const pointer>( ptr );
		}

		[[nodiscard]] pointer head() noexcept
		{
			return cast( m_head.m_next );
		}
		[[nodiscard]] const_pointer head() const noexcept
		{
			return cast( m_head.m_next );
		}

		[[nodiscard]] pointer before_begin_ptr() noexcept
		{
			return cast( &m_head );
		}
		[[nodiscard]] const_pointer before_begin_ptr() const noexcept
		{
			return cast( &m_head );
		}

		[[nodiscard]] static hook_type* unwrap_iterator( const_iterator it ) noexcept
		{
			return const_cast<hook_type*>( static_cast<const hook_type*>( it.m_data ) );
		}

		hook_type m_head;
	};
}

namespace std
{
	template <typename T, typename Tag, typename U = T>
	mclo::intrusive_forward_list<T, Tag>::size_type erase( mclo::intrusive_forward_list<T, Tag>& list, const U& value )
	{
		return list.remove( value );
	}

	template <typename T, typename Tag, typename Pred>
	mclo::intrusive_forward_list<T, Tag>::size_type erase_if( mclo::intrusive_forward_list<T, Tag>& list, Pred pred )
	{
		return list.remove_if( pred );
	}
}
