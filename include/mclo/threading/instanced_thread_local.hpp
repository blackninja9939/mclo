#pragma once

#include "mclo/threading/thread_local_key.hpp"

#include <atomic>
#include <memory>
#include <type_traits>

namespace mclo
{
	template <typename T>
	class instanced_thread_local
	{
		struct thread_data
		{
			T m_object{};
			thread_data* m_next = nullptr;
		};

	public:
		using value_type = T;

		class iterator
		{
		public:
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = value_type*;
			using reference = value_type&;
			using iterator_category = std::forward_iterator_tag;
			using iterator_concept = std::forward_iterator_tag;

			iterator() = default;

			[[nodiscard]] T& operator*() const noexcept
			{
				return m_data->m_object;
			}

			[[nodiscard]] T* operator->() const noexcept
			{
				return std::addressof( m_data->m_object );
			}

			iterator& operator++() noexcept
			{
				m_data = m_data->m_next;
				return *this;
			}

			iterator operator++( int ) noexcept
			{
				iterator temp( *this );
				++( *this );
				return temp;
			}

			[[nodiscard]] bool operator==( const iterator& other ) const noexcept = default;

		private:
			friend class instanced_thread_local;

			explicit iterator( thread_data* const data ) noexcept
				: m_data( data )
			{
			}

			thread_data* m_data = nullptr;
		};

		~instanced_thread_local()
		{
			thread_data* data = m_data_head.load( std::memory_order_relaxed );
			while ( data )
			{
				thread_data* const next = data->m_next;
				delete data;
				data = next;
			}
		}

		[[nodiscard]] T& get()
		{
			thread_data* data = static_cast<thread_data*>( m_key.get() );
			if ( !data )
			{
				data = new thread_data();
				m_key.set( data );

				data->m_next = m_data_head.load( std::memory_order_relaxed );
				while ( !m_data_head.compare_exchange_weak(
					data->m_next, data, std::memory_order_release, std::memory_order_relaxed ) )
				{
				}
			}
			return data->m_object;
		}

		[[nodiscard]] T& operator*()
		{
			return get();
		}

		[[nodiscard]] T* operator->()
		{
			return std::addressof( get() );
		}

		[[nodiscard]] iterator begin() noexcept
		{
			return iterator( m_data_head.load( std::memory_order_relaxed ) );
		}

		[[nodiscard]] iterator end() noexcept
		{
			return iterator();
		}

	private:
		std::atomic<thread_data*> m_data_head{ nullptr };
		thread_local_key m_key;
	};

	template <typename T>
	class instanced_thread_local_value
	{
	public:
		static_assert( sizeof( T ) <= sizeof( void* ), "T must be <= the size of a pointer" );
		static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable" );
		static_assert( std::is_trivially_destructible_v<T>, "T must be trivially destructible" );

		using value_type = T;

		[[nodiscard]] T get()
		{
			if constexpr ( std::is_pointer_v<T> )
			{
				return static_cast<T>( m_key.get() );
			}
			else
			{
				return static_cast<T>( reinterpret_cast<std::uintptr_t>( m_key.get() ) );
			}
		}

		void set( T value )
		{
			if constexpr ( std::is_pointer_v<T> )
			{
				m_key.set( value );
			}
			else
			{
				m_key.set( reinterpret_cast<void*>( static_cast<std::uintptr_t>( value ) ) );
			}
		}

	private:
		thread_local_key m_key;
	};
}
