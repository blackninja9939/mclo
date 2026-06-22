#pragma once

#include <cstddef>
#include <type_traits>

namespace mclo
{
	/// @brief A region-based (arena) allocator that hands out memory linearly from a chain of chunks.
	/// @details Allocations are served by bumping a pointer through the current chunk; when a chunk is exhausted a new,
	/// larger chunk is allocated and linked into the list. Individual allocations cannot be freed, only the whole arena
	/// can be reset or released, making allocation extremely cheap. Use @ref arena_allocator to plug an arena into
	/// standard containers.
	class memory_arena
	{
	public:
		/// @brief Constructs an empty arena that allocates its first chunk on the first allocation.
		memory_arena() = default;

		/// @brief Constructs an arena with an initial chunk of at least @p size bytes pre-allocated.
		/// @param size The number of bytes to reserve up front.
		explicit memory_arena( const std::size_t size )
		{
			grow( size );
		}

		memory_arena( const memory_arena& ) = delete;
		memory_arena& operator=( const memory_arena& ) = delete;

		memory_arena( memory_arena&& other ) noexcept;
		memory_arena& operator=( memory_arena&& other ) noexcept;

		~memory_arena()
		{
			release();
		}

		/// @brief Allocates @p size bytes aligned to @p alignment from the arena.
		/// @details Serves the allocation from the current chunk if it fits, otherwise grows the arena with a new
		/// chunk.
		/// @param size The number of bytes to allocate.
		/// @param alignment The required alignment of the returned pointer.
		/// @return A pointer to the allocated memory.
		void* allocate( const std::size_t size, std::size_t alignment = alignof( std::max_align_t ) );

		/// @brief Resets the arena so all chunks can be reused, keeping the allocated chunks for future allocations.
		/// @details Does not return memory to the system; subsequent allocations reuse the existing chunks.
		void reset() noexcept;

		/// @brief Resets the arena and consolidates all chunks into a single chunk large enough to hold them.
		/// @details Releases the existing chunks and allocates one chunk sized to the total previous capacity, reducing
		/// fragmentation for the next round of allocations.
		void reset_consolidate();

		/// @brief Releases all chunks back to the system, leaving the arena empty.
		void release() noexcept;

	private:
		struct chunk final
		{
			chunk* m_next = nullptr;
			std::size_t m_size = 0;

			std::byte* begin() noexcept
			{
				return reinterpret_cast<std::byte*>( this ) + sizeof( chunk );
			}
			std::byte* end() noexcept
			{
				return begin() + m_size;
			}
		};

		[[nodiscard]] static chunk* allocate_chunk( const std::size_t size );

		void grow( const std::size_t size );

		std::byte* m_current = nullptr;
		chunk* m_current_chunk = nullptr;
		chunk* m_head = nullptr;
	};

	/// @brief A standard-library-compatible allocator that allocates from a @ref memory_arena.
	/// @details Satisfies the @c Allocator requirements so it can be used with standard containers. @ref deallocate is
	/// a no-op since the backing arena reclaims memory only in bulk via @ref memory_arena::reset or
	/// @ref memory_arena::release.
	/// @tparam T The element type to allocate.
	template <typename T>
	class arena_allocator
	{
	public:
		using value_type = T;
		using is_always_equal = std::false_type;

		/// @brief Constructs an allocator that allocates from @p arena.
		/// @param arena The arena to allocate from; must outlive this allocator and any container using it.
		arena_allocator( memory_arena& arena ) noexcept
			: m_arena( &arena )
		{
		}

		/// @brief Rebinding constructor allowing the allocator to be used for a different element type @p U.
		/// @param other The allocator to copy the backing arena from.
		template <typename U>
		arena_allocator( const arena_allocator<U>& other ) noexcept
			: m_arena( &other.arena() )
		{
		}

		/// @brief Compares two allocators for equality, true when they share the same arena.
		template <typename U>
		bool operator==( const arena_allocator<U>& other ) const noexcept
		{
			return m_arena == &other.arena();
		}

		/// @brief Compares two allocators for inequality.
		template <typename U>
		bool operator!=( const arena_allocator<U>& other ) const noexcept
		{
			return !( *this == other );
		}

		/// @brief Returns true if this allocator allocates from @p arena.
		bool operator==( const memory_arena& arena ) const noexcept
		{
			return m_arena == &arena;
		}

		/// @brief Allocates storage for @p n objects of type @p T from the arena.
		/// @param n The number of objects to allocate.
		/// @return A pointer to the allocated storage.
		[[nodiscard]] T* allocate( const std::size_t n )
		{
			return static_cast<T*>( m_arena->allocate( n * sizeof( T ), alignof( T ) ) );
		}

		/// @brief No-op deallocation; arena memory is reclaimed only in bulk by the @ref memory_arena.
		void deallocate( T*, std::size_t ) noexcept
		{
		}

		/// @brief Returns the @ref memory_arena this allocator allocates from.
		memory_arena& arena() const noexcept
		{
			return *m_arena;
		}

	private:
		memory_arena* m_arena;
	};
}
