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

		void push_front( reference value ) noexcept
		{
			insert_after( before_begin(), value );
		}

		[[nodiscard]] pointer pop_front() noexcept
		{
			return cast( unwrap_iterator( erase_after( before_begin() ) ) );
		}

		iterator insert_after( const_iterator pos, reference value ) noexcept
		{
			DEBUG_ASSERT( pos != end(), "Cannot insert after end of forward list" );
			hook_type* const ptr = unwrap_iterator( pos );
			hook_type* const hook = &value;
			hook->m_next = std::exchange( ptr->m_next, hook );
			return iterator( &value );
		}

		template <std::input_iterator It>
		iterator insert_after( const_iterator pos, It first, It last ) noexcept;

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

		void splice_after( const_iterator pos, intrusive_forward_list& other ) noexcept
		{
			DEBUG_ASSERT( this != &other, "Cannot splice in same container" );
			DEBUG_ASSERT( pos != end(), "Cannot splice after end of forward list" );

			// exchange empties other
			hook_type* const hook = std::exchange( other.m_head.m_next, nullptr );

			if ( hook )
			{
				hook_type* const ptr = unwrap_iterator( pos );
				hook->m_next = std::exchange( ptr->m_next, hook );
			}
		}

		void splice_after( const_iterator pos, intrusive_forward_list&& other ) noexcept
		{
			splice_after( pos, other );
		}

		// todo(mc) other splice_after overloads

		size_type remove( const_reference value )
		{
			return remove( [ &value ]( const_reference object ) { return object == value; } );
		}

		template <std::predicate<const_reference> UnaryPredicate>
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

		void reverse() noexcept;

		template <std::invocable<pointer> Func>
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
			consume( []( pointer ) {} );
		}

	private:
		static pointer cast( hook_type* ptr ) noexcept
		{
			return static_cast<pointer>( ptr );
		}
		static const_pointer cast( const hook_type* ptr ) noexcept
		{
			return static_cast<const pointer>( ptr );
		}

		pointer head() noexcept
		{
			return cast( m_head.m_next );
		}
		const_pointer head() const noexcept
		{
			return cast( m_head.m_next );
		}

		pointer before_begin_ptr() noexcept
		{
			return cast( &m_head );
		}
		const_pointer before_begin_ptr() const noexcept
		{
			return cast( &m_head );
		}

		static hook_type* unwrap_iterator( const_iterator it ) noexcept
		{
			return const_cast<hook_type*>( static_cast<const hook_type*>( it.m_data ) );
		}

		hook_type m_head;
	};
}
