#pragma once

#include "mclo/container/detail/intrusive_forward_list_iterator.hpp"
#include "mclo/container/intrusive_forward_list_hook.hpp"

#include <atomic>

namespace mclo
{
	/// @brief A lock-free, intrusive, singly linked list with atomic insertion and removal at the front.
	/// @details Nodes are linked intrusively: each element must derive from @c intrusive_forward_list_hook, so the list
	/// performs no allocation of its own and never owns its elements. The @c push_front, @c pop_front and @c consume
	/// operations are safe to call concurrently from multiple threads. The list is also iterable, taking an atomic
	/// snapshot of the head, which is useful for inspecting or aggregating elements.
	/// @tparam T The element type. Must derive from @c intrusive_forward_list_hook<Tag>.
	/// @tparam Tag A tag type allowing a single element to participate in multiple independent lists simultaneously.
	/// @warning For a given @p Tag an element may only belong to one list at a time.
	/// @warning Iteration is unsynchronised against concurrent mutation: elements pushed by another thread after the
	/// snapshot is taken may not be observed, and you must not iterate while any of the elements you do observe are
	/// being modified by their owning threads.
	template <typename T, typename Tag = void>
	class atomic_intrusive_forward_list
	{
		using hook_type = intrusive_forward_list_hook<Tag>;
		static_assert( std::derived_from<T, hook_type>, "T must be derived from the intrusive list hook" );

	public:
		/// @brief The type of elements stored in the list.
		using value_type = T;
		/// @brief The unsigned integer type used for sizes.
		using size_type = std::size_t;
		/// @brief The signed integer type used for differences between iterators.
		using difference_type = std::ptrdiff_t;
		/// @brief A reference to an element.
		using reference = value_type&;
		/// @brief A reference to a constant element.
		using const_reference = const value_type&;
		/// @brief A pointer to an element.
		using pointer = value_type*;
		/// @brief A pointer to a constant element.
		using const_pointer = const value_type*;
		/// @brief A mutable forward iterator over the elements of the list.
		using iterator = intrusive_forward_list_iterator<T, Tag>;
		/// @brief A constant forward iterator over the elements of the list.
		using const_iterator = intrusive_forward_list_iterator<const T, Tag>;

		/// @brief Constructs an empty list.
		atomic_intrusive_forward_list() noexcept = default;

		atomic_intrusive_forward_list( const atomic_intrusive_forward_list& other ) = delete;
		atomic_intrusive_forward_list& operator=( const atomic_intrusive_forward_list& other ) = delete;

		/// @brief Move constructs by atomically taking ownership of the other list's elements.
		/// @param other The list to take the elements from, left empty afterwards.
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

		/// @brief Returns an iterator to the first element, taking an atomic snapshot of the head.
		/// @return An iterator to the beginning of the list.
		[[nodiscard]] iterator begin() noexcept
		{
			return iterator( head() );
		}
		/// @brief Returns a constant iterator to the first element, taking an atomic snapshot of the head.
		/// @return A constant iterator to the beginning of the list.
		[[nodiscard]] const_iterator begin() const noexcept
		{
			return const_iterator( head() );
		}
		/// @brief Returns a constant iterator to the first element, taking an atomic snapshot of the head.
		/// @return A constant iterator to the beginning of the list.
		[[nodiscard]] const_iterator cbegin() const noexcept
		{
			return const_iterator( head() );
		}

		/// @brief Returns an iterator past the last element.
		/// @return An iterator to the end of the list.
		[[nodiscard]] iterator end() noexcept
		{
			return iterator( nullptr );
		}
		/// @brief Returns a constant iterator past the last element.
		/// @return A constant iterator to the end of the list.
		[[nodiscard]] const_iterator end() const noexcept
		{
			return const_iterator( nullptr );
		}
		/// @brief Returns a constant iterator past the last element.
		/// @return A constant iterator to the end of the list.
		[[nodiscard]] const_iterator cend() const noexcept
		{
			return const_iterator( nullptr );
		}

		/// @brief Checks whether the list currently has no elements.
		/// @return @c true if the list is empty, otherwise @c false.
		[[nodiscard]] bool empty() const noexcept
		{
			return m_head.load( std::memory_order_relaxed ) == nullptr;
		}

		/// @brief Atomically inserts an element at the front of the list.
		/// @param value The element to insert. Its lifetime must outlast its membership in the list.
		/// @warning The element must not already belong to another list using the same @p Tag.
		void push_front( reference value ) noexcept
		{
			hook_type& hook = value;
			hook.m_next = m_head.load( std::memory_order_relaxed );
			while ( !m_head.compare_exchange_weak(
				hook.m_next, &hook, std::memory_order_release, std::memory_order_relaxed ) )
			{
			}
		}

		/// @brief Atomically removes and returns the element at the front of the list.
		/// @return A pointer to the removed element, or @c nullptr if the list was empty.
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

		/// @brief Atomically detaches all elements and invokes a callable on each of them.
		/// @details Exchanges the head for null in a single atomic operation, then walks the detached chain invoking
		/// @p func with a pointer to each element. Useful for draining the list, for example to destroy or recycle
		/// every node.
		/// @tparam Func A callable accepting a @c pointer. Must be nothrow invocable.
		/// @param func The callable invoked once with each detached element.
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

		/// @brief Atomically detaches and discards all elements.
		/// @note As the list is non-owning the elements themselves are left untouched.
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
