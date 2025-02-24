#pragma once

#include "mclo/preprocessor/platform.hpp"
#include "mclo/threading/thread_local_key.hpp"

#include <atomic>
#include <memory>
#include <type_traits>

namespace mclo
{
	template <typename T, typename Allocator = std::allocator<T>>
	class instanced_thread_local
	{
		struct thread_data
		{
			T m_object{};
			thread_data* m_next = nullptr;
		};

		using thread_data_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<thread_data>;

	public:
		using value_type = T;
		using allocator_type = Allocator;

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

		static_assert( std::is_default_constructible_v<value_type>, "T must be default constructible" );

		instanced_thread_local() noexcept( std::is_nothrow_default_constructible_v<allocator_type> ) = default;

		explicit instanced_thread_local( const allocator_type& allocator ) noexcept
			: m_allocator( allocator )
		{
		}

		~instanced_thread_local()
		{
			thread_data_allocator alloc( m_allocator );
			thread_data* data = m_data_head.load( std::memory_order_relaxed );
			while ( data )
			{
				thread_data* const next = data->m_next;
				std::destroy_at( data );
				alloc.deallocate( data, 1 );
				data = next;
			}
		}

		[[nodiscard]] T& get()
		{
			thread_data* data = static_cast<thread_data*>( m_key.get() );
			if ( !data )
			{
				data = create_thread_data();
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

		allocator_type get_allocator() const noexcept
		{
			return m_allocator;
		}

	private:
		[[nodiscard]] thread_data* create_thread_data()
		{
			thread_data_allocator alloc( m_allocator );
			thread_data* data = alloc.allocate( 1 );
			try
			{
				data = std::construct_at( data );
			}
			catch ( ... )
			{
				alloc.deallocate( data, 1 );
				throw;
			}
			return data;
		}

		std::atomic<thread_data*> m_data_head{ nullptr };
		thread_local_key m_key;
		MCLO_NO_UNIQUE_ADDRESS allocator_type m_allocator;
	};

	template <typename T>
	class instanced_thread_local_value
	{
	public:
		static_assert( sizeof( T ) <= sizeof( void* ), "T must be <= the size of a pointer" );
		static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable" );
		static_assert( std::is_trivially_destructible_v<T>, "T must be trivially destructible" );

		using value_type = T;

		[[nodiscard]] T get() noexcept
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
