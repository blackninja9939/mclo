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
	class typed_memory_pool : public memory_pool
	{
	public:
		/// @brief Constructs a pool of @p chunk_count chunks each sized and aligned for type @p T.
		/// @param chunk_count The number of @p T-sized chunks in the pool.
		explicit typed_memory_pool( const std::size_t chunk_count )
			: memory_pool( chunk_count, sizeof( T ), alignof( T ) )
		{
		}
	};

	/// @brief A standard-library-compatible allocator that allocates from a @ref memory_pool.
	/// @details Satisfies the @c Allocator requirements so it can be used with standard containers. Each allocation and
	/// deallocation pushes to or pops from the pool's free list, making it suited to containers that allocate a single
	/// fixed-size node at a time (e.g. node-based containers).
	/// @tparam T The element type to allocate.
	template <typename T>
	class pool_allocator
	{
	public:
		using value_type = T;
		using is_always_equal = std::false_type;

		/// @brief Constructs an allocator that allocates from @p pool.
		/// @param pool The pool to allocate from; must outlive this allocator and any container using it.
		pool_allocator( memory_pool& pool ) noexcept
			: m_pool( &pool )
		{
		}

		/// @brief Rebinding constructor allowing the allocator to be used for a different element type @p U.
		/// @param other The allocator to copy the backing pool from.
		template <typename U>
		pool_allocator( const pool_allocator<U>& other ) noexcept
			: m_pool( &other.pool() )
		{
		}

		/// @brief Compares two allocators for equality, true when they share the same pool.
		template <typename U>
		bool operator==( const pool_allocator<U>& other ) const noexcept
		{
			return m_pool == &other.pool();
		}

		/// @brief Compares two allocators for inequality.
		template <typename U>
		bool operator!=( const pool_allocator<U>& other ) const noexcept
		{
			return !( *this == other );
		}

		/// @brief Returns true if this allocator allocates from @p pool.
		bool operator==( const memory_pool& pool ) const noexcept
		{
			return m_pool == &pool;
		}

		/// @brief Allocates storage for @p n objects of type @p T from the pool.
		/// @param n The number of objects to allocate.
		/// @return A pointer to the allocated storage.
		[[nodiscard]] T* allocate( const std::size_t n )
		{
			return static_cast<T*>( m_pool->allocate( n * sizeof( T ), alignof( T ) ) );
		}

		/// @brief Returns the storage for @p n objects of type @p T pointed to by @p ptr to the pool.
		/// @param ptr The pointer previously returned by @ref allocate.
		/// @param n The number of objects originally allocated.
		void deallocate( T* const ptr, const std::size_t n ) noexcept
		{
			m_pool->deallocate( ptr, n * sizeof( T ), alignof( T ) );
		}

		/// @brief Returns the @ref memory_pool this allocator allocates from.
		memory_pool& pool() const noexcept
		{
			return *m_pool;
		}

	private:
		memory_pool* m_pool;
	};
}
