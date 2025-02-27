#pragma once

#include "mclo/preprocessor/platform.hpp"
#include "mclo/threading/atomic_intrusive_forward_list.hpp"
#include "mclo/threading/thread_local_key.hpp"

#include <atomic>
#include <memory>
#include <type_traits>

namespace mclo
{
	template <typename T, typename Allocator = std::allocator<T>>
	class instanced_thread_local
	{
		// Aligned to cache line to avoids false sharing of the actual object in case two threads allocated on same
		// cache line
		struct alignas( std::hardware_destructive_interference_size ) thread_data : intrusive_forward_list_hook<>
		{
			T m_object{};
		};

		using thread_data_list = atomic_intrusive_forward_list<thread_data>;
		using list_iterator = typename thread_data_list::iterator;
		using thread_data_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<thread_data>;

	public:
		using value_type = T;
		using allocator_type = Allocator;

		class iterator : public list_iterator
		{
		public:
			explicit iterator( list_iterator wrapped ) noexcept
				: list_iterator( wrapped )
			{
			}

			using value_type = T;
			using pointer = T*;
			using reference = T&;

			[[nodiscard]] reference operator*() const noexcept
			{
				return static_cast<const list_iterator&>( *this )->m_object;
			}

			[[nodiscard]] pointer operator->() const noexcept
			{
				return std::addressof( **this );
			}
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
			m_list.consume( [ &alloc ]( thread_data* ptr ) noexcept {
				std::destroy_at( ptr );
				alloc.deallocate( ptr, 1 );
			} );
		}

		[[nodiscard]] T& get()
		{
			thread_data* data = static_cast<thread_data*>( m_key.get() );
			if ( !data )
			{
				data = create_thread_data();
				m_key.set( data );
				m_list.push_front( *data );
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
			return iterator( m_list.begin() );
		}

		[[nodiscard]] iterator end() noexcept
		{
			return iterator( m_list.end() );
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

		thread_data_list m_list;
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
