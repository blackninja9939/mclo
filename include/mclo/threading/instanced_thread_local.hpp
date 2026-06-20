#pragma once

#include "mclo/platform/attributes.hpp"
#include "mclo/platform/warnings.hpp"
#include "mclo/threading/atomic_intrusive_forward_list.hpp"
#include "mclo/threading/thread_local_key.hpp"

#include <atomic>
#include <memory>
#include <memory_resource>
#include <type_traits>

namespace mclo
{
	/// @brief A thread-local object that is instanced per owning container, allowing iteration over every thread's
	/// copy.
	/// @details Unlike the @c thread_local keyword, which creates a single object shared by all instances at a given
	/// scope, each @c instanced_thread_local creates a distinct per-thread object for that specific instance. This
	/// allows many independent thread-local containers to exist simultaneously. Each thread lazily allocates its own
	/// object on first access via @c get(), and all live per-thread objects are linked together so they can be
	/// iterated, for example to aggregate per-thread results. Per-thread objects are cache-line aligned to avoid false
	/// sharing.
	/// @tparam T The type of the per-thread object. Must be default constructible.
	/// @tparam Allocator The allocator used to allocate per-thread objects.
	/// @warning Iteration is not synchronised against concurrent access by other threads. You must only iterate once
	/// you are certain that no other thread is still writing to its own object, since iteration reads each thread's
	/// value without locking. Additionally, threads that have not yet accessed the object will not be visible, so newly
	/// joining threads may be missed entirely.
	template <typename T, typename Allocator = std::allocator<T>>
	class instanced_thread_local
	{
		// Aligned to cache line to avoids false sharing of the actual object in case two threads allocated on same
		// cache line
		MCLO_DISABLE_WARNINGS( MCLO_WARNING_ALIGNMENT_PADDING )
		struct alignas( std::hardware_destructive_interference_size ) thread_data : intrusive_forward_list_hook<>
		{
			template <typename... Ts>
			thread_data( Ts&&... args ) noexcept( std::is_nothrow_constructible_v<T, Ts...> )
				: m_object( std::forward<Ts>( args )... )
			{
			}

			T m_object{};
		};
		MCLO_RESTORE_WARNINGS

		using thread_data_list = atomic_intrusive_forward_list<thread_data>;
		using list_iterator = typename thread_data_list::iterator;
		using thread_data_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<thread_data>;

	public:
		using value_type = T;
		using allocator_type = Allocator;

		/// @brief A forward iterator over every thread's instance of the object.
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

		/// @brief Constructs the container using the given allocator.
		/// @param allocator The allocator used to allocate per-thread objects.
		explicit instanced_thread_local( const allocator_type& allocator ) noexcept
			: m_allocator( allocator )
		{
		}

		~instanced_thread_local()
		{
			thread_data_allocator alloc( m_allocator );
			m_list.consume( [ &alloc ]( thread_data* ptr ) noexcept {
				std::allocator_traits<thread_data_allocator>::destroy( alloc, ptr );
				alloc.deallocate( ptr, 1 );
			} );
		}

		/// @brief Retrieves the calling thread's object, lazily creating it on first access.
		/// @details The construction arguments are only used the first time this thread accesses the object; subsequent
		/// calls from the same thread ignore them and return the existing object.
		/// @param construct_args Arguments forwarded to the constructor of @c T when this thread's object is first
		/// created.
		/// @return A reference to the calling thread's object.
		template <typename... Ts>
		[[nodiscard]] T& get( Ts&&... construct_args )
		{
			thread_data* data = static_cast<thread_data*>( m_key.get() );
			if ( !data )
			{
				data = create_thread_data( std::forward<Ts>( construct_args )... );
				m_key.set( data );
				m_list.push_front( *data );
			}
			return data->m_object;
		}

		/// @brief Retrieves the calling thread's object, lazily default-creating it on first access.
		/// @return A reference to the calling thread's object.
		[[nodiscard]] T& operator*()
		{
			return get();
		}

		/// @brief Accesses the calling thread's object, lazily default-creating it on first access.
		/// @return A pointer to the calling thread's object.
		[[nodiscard]] T* operator->()
		{
			return std::addressof( get() );
		}

		/// @brief Returns an iterator to the first thread's object.
		/// @return An iterator to the beginning of the range of per-thread objects.
		[[nodiscard]] iterator begin() noexcept
		{
			return iterator( m_list.begin() );
		}

		/// @brief Returns an iterator past the last thread's object.
		/// @return An iterator to the end of the range of per-thread objects.
		[[nodiscard]] iterator end() noexcept
		{
			return iterator( m_list.end() );
		}

		/// @brief Returns the allocator used to allocate per-thread objects.
		/// @return A copy of the allocator.
		allocator_type get_allocator() const noexcept
		{
			return m_allocator;
		}

	private:
		template <typename... Ts>
		[[nodiscard]] thread_data* create_thread_data( Ts&&... args )
		{
			thread_data_allocator alloc( m_allocator );
			thread_data* data = alloc.allocate( 1 );
			try
			{
				std::allocator_traits<thread_data_allocator>::construct( alloc, data, std::forward<Ts>( args )... );
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

	/// @brief A lightweight per-instance thread-local holder for a small, trivially copyable value.
	/// @details Stores the value directly inside a single thread-local storage slot rather than allocating a separate
	/// per-thread object, making it cheaper than @c instanced_thread_local but limited to values that fit within a
	/// pointer. Unlike @c instanced_thread_local it does not track or allow iteration over the values stored by other
	/// threads. Each thread sees its own independent value, zero-initialised until first set.
	/// @tparam T The stored value type. Must be trivially copyable, trivially destructible, and no larger than a
	/// pointer.
	/// @see instanced_thread_local
	template <typename T>
	class instanced_thread_local_value
	{
	public:
		static_assert( sizeof( T ) <= sizeof( void* ), "T must be <= the size of a pointer" );
		static_assert( std::is_trivially_copyable_v<T>, "T must be trivially copyable" );
		static_assert( std::is_trivially_destructible_v<T>, "T must be trivially destructible" );

		using value_type = T;

		/// @brief Retrieves the calling thread's value.
		/// @return The value previously stored by this thread, or a zero-initialised value if none has been set.
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

		/// @brief Sets the value stored for the calling thread.
		/// @param value The value to store for the current thread.
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

	namespace pmr
	{
		/// @brief Alias for @c instanced_thread_local using a polymorphic allocator.
		template <typename T>
		using instanced_thread_local = mclo::instanced_thread_local<T, std::pmr::polymorphic_allocator<T>>;
	}
}
