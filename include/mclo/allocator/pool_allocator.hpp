#pragma once

#include <cstddef>

#include "mclo/threading/atomic_intrusive_forward_list.hpp"

namespace mclo
{
	/// @brief Thread safe fixed size and alignment memory pool
	/// @details Allocates a single buffer of chunk_count * chunk_size bytes aligned to chunk_alignment, creates
	/// chunk_count chunks of the pool into a free list which will be pushed and popped to.
	/// @warning The size and alignment will be rounded up to the internal free list header requirement, so this is not
	/// very efficient for pools of small types less than a pointer in size and align.
	class memory_pool
	{
	public:
		memory_pool( const std::size_t chunk_count, const std::size_t chunk_size, const std::size_t chunk_alignment );
		~memory_pool();

		memory_pool( const memory_pool& ) = delete;
		memory_pool& operator=( const memory_pool& ) = delete;

		memory_pool( memory_pool&& other ) noexcept;
		memory_pool& operator=( memory_pool&& other ) noexcept = delete;

		void* allocate( const std::size_t size, const std::size_t alignment = alignof( std::max_align_t ) );
		void deallocate( void* ptr,
						 const std::size_t size,
						 const std::size_t alignment = alignof( std::max_align_t ) ) noexcept;

	private:
		using free_list_node = mclo::intrusive_forward_list_hook<>;

		std::size_t m_chunk_alignment = 0;
		std::size_t m_chunk_size = 0;
		std::byte* m_data = nullptr;
		mclo::atomic_intrusive_forward_list<free_list_node> m_free_list;
	};

	template <typename T>
	class pool_allocator
	{
	public:
		using value_type = T;
		using is_always_equal = std::false_type;

		pool_allocator( memory_pool& pool ) noexcept
			: m_pool( &pool )
		{
		}

		template <typename U>
		pool_allocator( const pool_allocator<U>& other ) noexcept
			: m_pool( &other.pool() )
		{
		}

		template <typename U>
		bool operator==( const pool_allocator<U>& other ) const noexcept
		{
			return m_pool == &other.pool();
		}

		template <typename U>
		bool operator!=( const pool_allocator<U>& other ) const noexcept
		{
			return !( *this == other );
		}

		bool operator==( const memory_pool& pool ) const noexcept
		{
			return m_pool == &pool;
		}

		[[nodiscard]] T* allocate( const std::size_t n )
		{
			return static_cast<T*>( m_pool->allocate( n * sizeof( T ), alignof( T ) ) );
		}

		void deallocate( T* const ptr, const std::size_t n ) noexcept
		{
			m_pool->deallocate( ptr, n * sizeof( T ), alignof( T ) );
		}

		memory_pool& pool() const noexcept
		{
			return *m_pool;
		}

	private:
		memory_pool* m_pool;
	};
}
