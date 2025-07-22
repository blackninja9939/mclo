#include "mclo/allocator/arena_allocator.hpp"

#include <memory>
#include <new>
#include <utility>

namespace mclo
{
	memory_arena::memory_arena( memory_arena&& other ) noexcept
		: m_head( std::exchange( other.m_head, nullptr ) )
		, m_current_chunk( std::exchange( other.m_current_chunk, nullptr ) )
		, m_current( std::exchange( other.m_current, nullptr ) )
	{
	}

	memory_arena& memory_arena::operator=( memory_arena&& other ) noexcept
	{
		if ( this != &other )
		{
			release();
			m_head = std::exchange( other.m_head, nullptr );
			m_current_chunk = std::exchange( other.m_current_chunk, nullptr );
			m_current = std::exchange( other.m_current, nullptr );
		}
		return *this;
	}

	void* memory_arena::allocate( const std::size_t size, std::size_t alignment /*= alignof( std::max_align_t ) */ )
	{
		void* ptr = m_current;
		std::size_t space = static_cast<std::size_t>( m_current_chunk->end() - m_current );

		if ( !std::align( alignment, size, ptr, space ) )
		{
			grow( std::max( size + alignment, m_current_chunk->m_size * 2 ) );
			ptr = m_current;
			space = static_cast<std::size_t>( m_current_chunk->end() - m_current );

			if ( !std::align( alignment, size, ptr, space ) )
			{
				throw std::bad_alloc(); // even after growing? Should not happen.
			}
		}

		m_current = static_cast<std::byte*>( ptr ) + size;
		return ptr;
	}

	void memory_arena::reset() noexcept
	{
		m_current_chunk = m_head;
		m_current = m_current_chunk->begin();
	}

	void memory_arena::reset_consolidate()
	{
		std::size_t toal_size = 0;

		chunk* head = m_head;
		while ( head )
		{
			toal_size += head->m_size;
			chunk* const next = head->m_next;
			::operator delete( head, std::align_val_t{ alignof( chunk ) } );
			head = next;
		}

		m_head = allocate_chunk( toal_size );
		m_current_chunk = m_head;
		m_current = m_head->begin();
	}

	void memory_arena::release() noexcept
	{
		chunk* head = m_head;
		while ( head )
		{
			chunk* const next = head->m_next;
			::operator delete( head, std::align_val_t{ alignof( chunk ) } );
			head = next;
		}

		m_head = nullptr;
		m_current_chunk = nullptr;
		m_current = nullptr;
	}

	memory_arena::chunk* memory_arena::allocate_chunk( const std::size_t size )
	{
		const std::size_t total_size = sizeof( chunk ) + size;
		void* const raw = ::operator new( total_size, std::align_val_t{ alignof( chunk ) } );
		if ( !raw )
		{
			throw std::bad_alloc();
		}

		return std::construct_at( static_cast<chunk*>( raw ), nullptr, size );
	}

	void memory_arena::grow( const std::size_t size )
	{
		chunk* const new_chunk = allocate_chunk( size );
		new_chunk->m_next = m_head;
		m_head = new_chunk;

		m_current_chunk = new_chunk;
		m_current = new_chunk->begin();
	}
}
