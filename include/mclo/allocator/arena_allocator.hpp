#pragma once

#include <cstddef>
#include <type_traits>

namespace mclo
{
	class memory_arena
	{
	public:
		memory_arena() = default;

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

		void* allocate( const std::size_t size, std::size_t alignment = alignof( std::max_align_t ) );

		void reset() noexcept;
		void reset_consolidate();

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

	template <typename T>
	class arena_allocator
	{
	public:
		using value_type = T;
		using is_always_equal = std::false_type;

		arena_allocator( memory_arena& arena ) noexcept
			: m_arena( &arena )
		{
		}

		template <typename U>
		arena_allocator( const arena_allocator<U>& other ) noexcept
			: m_arena( &other.arena() )
		{
		}

		template <typename U>
		bool operator==( const arena_allocator<U>& other ) const noexcept
		{
			return m_arena == &other.arena();
		}

		template <typename U>
		bool operator!=( const arena_allocator<U>& other ) const noexcept
		{
			return !( *this == other );
		}

		bool operator==( const memory_arena& arena ) const noexcept
		{
			return m_arena == &arena;
		}

		[[nodiscard]] T* allocate( const std::size_t n )
		{
			return static_cast<T*>( m_arena->allocate( n * sizeof( T ), alignof( T ) ) );
		}

		void deallocate( T*, std::size_t ) noexcept
		{
		}

		memory_arena& arena() const noexcept
		{
			return *m_arena;
		}

	private:
		memory_arena* m_arena;
	};
}
