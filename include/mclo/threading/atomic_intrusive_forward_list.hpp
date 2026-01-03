#pragma once

#include "mclo/container/detail/intrusive_forward_list_iterator.hpp"
#include "mclo/container/intrusive_forward_list_hook.hpp"

#include <atomic>

namespace mclo
{
	template <typename T, typename Tag = void>
	class atomic_intrusive_forward_list
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

		atomic_intrusive_forward_list() noexcept = default;

		atomic_intrusive_forward_list( const atomic_intrusive_forward_list& other ) = delete;
		atomic_intrusive_forward_list& operator=( const atomic_intrusive_forward_list& other ) = delete;

		atomic_intrusive_forward_list( atomic_intrusive_forward_list&& other ) noexcept
			: m_head( other.m_head.exchange( nullptr, std::memory_order_acq_rel ) )
		{
		}

		// Hard to implement and of limited use in practice
		atomic_intrusive_forward_list& operator=( atomic_intrusive_forward_list&& other ) noexcept = delete;

		~atomic_intrusive_forward_list()
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

		[[nodiscard]] bool empty() const noexcept
		{
			return m_head.load( std::memory_order_relaxed ) == nullptr;
		}

		void push_front( reference value ) noexcept
		{
			hook_type& hook = value;
			hook.m_next = m_head.load( std::memory_order_relaxed );
			while ( !m_head.compare_exchange_weak(
				hook.m_next, &hook, std::memory_order_release, std::memory_order_relaxed ) )
			{
			}
		}

		[[nodiscard]] pointer pop_front() noexcept
		{
			hook_type* head = m_head.load( std::memory_order_acquire );

			while ( head )
			{
				hook_type* const next = head->m_next;

				if ( m_head.compare_exchange_weak( head, next, std::memory_order_acquire, std::memory_order_relaxed ) )
				{
					// Detach the node from the list
					head->m_next = nullptr;
					return cast( head );
				}
			}

			return nullptr;
		}

		template <std::invocable<pointer> Func>
			requires( std::is_nothrow_invocable_v<Func, pointer> )
		void consume( Func func ) noexcept
		{
			hook_type* head = m_head.exchange( nullptr, std::memory_order_acq_rel );
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

	private:
		static pointer cast( hook_type* ptr ) noexcept
		{
			return static_cast<pointer>( ptr );
		}

		pointer head() const noexcept
		{
			return cast( m_head.load( std::memory_order_acquire ) );
		}

		std::atomic<hook_type*> m_head{ nullptr };
	};
}
