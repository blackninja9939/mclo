#pragma once

#include "mclo/container/detail/intrusive_forward_list_iterator.hpp"
#include "mclo/container/intrusive_forward_list_hook.hpp"
#include "mclo/debug/assert.hpp"

#include <utility>

namespace mclo
{
	template <typename T, typename Tag = void>
	class intrusive_forward_list
	{
		using hook_type = intrusive_forward_list_hook<Tag>;
		static_assert( std::derived_from<T, hook_type>, "T must be derived from the intrusive list hook" );

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
			: m_head( std::exchange( other.m_head, nullptr ) )
		{
		}
		intrusive_forward_list& operator=( const intrusive_forward_list& other ) noexcept
		{
			if ( this != &other )
			{
				m_head = std::exchange( other.m_head, nullptr );
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
			return m_head == nullptr;
		}

		void push_front( reference value ) noexcept
		{
			hook_type& hook = value;
			hook.m_next = m_head;
			m_head = &hook;
		}

		[[nodiscard]] pointer pop_front() noexcept
		{
			if ( !m_head )
			{
				return nullptr;
			}
			return cast( std::exchange( m_head, m_head->m_next ) );
		}

		iterator insert_after( const_iterator pos, reference value ) noexcept
		{
			DEBUG_ASSERT( pos != end(), "Cannot insert after end of forward list" );
			const pointer ptr = std::to_address( pos );
			hook_type* const hook = &value;
			hook->m_next = std::exchange( ptr->m_next, hook );
			return iterator( &value );
		}

		iterator erase_after( const_iterator pos ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos != end(), "Cannot erase after end of forward list" );
			const pointer ptr = std::to_address( pos );
			if ( ptr->m_next )
			{
				ptr->m_next = ptr->m_next->m_next;
			}
			return iterator( cast( ptr->m_next ) );
		}

		void splice_after( const_iterator pos, intrusive_forward_list& other ) noexcept
		{
			DEBUG_ASSERT( this != &other, "Cannot splice in same container" );
			DEBUG_ASSERT( pos != end(), "Cannot splice after end of forward list" );

			// exchange empties other
			hook_type* const hook = std::exchange( other.m_head, nullptr );

			if ( hook )
			{
				const pointer ptr = std::to_address( pos );
				hook->m_next = std::exchange( ptr->m_next, hook );
			}
		}

		void splice_after( const_iterator pos, intrusive_forward_list&& other ) noexcept
		{
			splice_after( pos, other );
		}

		template <std::invocable<pointer> Func>
		void consume( Func func ) noexcept
		{
			hook_type* head = std::exchange( m_head, nullptr );
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

		pointer head() const noexcept
		{
			return cast( m_head );
		}

		pointer before_begin_ptr() const noexcept
		{
			return std::addressof( reinterpret_cast<hook_type&>( const_cast<hook_type*&>( m_head ) ) );
		}

		hook_type* m_head{ nullptr };
	};
}
