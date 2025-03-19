#include "mclo/allocator/linear_allocator.hpp"

namespace
{
	std::byte* align( size_t alignment, size_t size, std::byte*& ptr, const std::byte* end ) noexcept
	{
		std::size_t offset = static_cast<size_t>( reinterpret_cast<uintptr_t>( ptr ) & ( alignment - 1 ) );
		if ( offset != 0 )
		{
			offset = alignment - offset; // number of bytes to skip
		}

		if ( static_cast<std::size_t>( end - ptr ) < offset + size )
		{
			return nullptr;
		}

		// enough room, update
		ptr += offset;
		return ptr;
	}

	[[nodiscard]] std::size_t scaled_increase( const std::size_t bytes_needed, const std::size_t existing_size )
	{
		const auto grown_size = static_cast<std::size_t>( existing_size * 1.5f );
		return std::max( grown_size, bytes_needed );
	}
}

namespace mclo
{
	linear_allocator_resource::linear_allocator_resource( const std::size_t initial_size,
														  std::pmr::memory_resource& upstream )
		: m_upstream( &upstream )
	{
		add_buffer_node( initial_size );
	}

	linear_allocator_resource::linear_allocator_resource( const std::size_t initial_size )
		: linear_allocator_resource( initial_size, *std::pmr::get_default_resource() )
	{
	}

	linear_allocator_resource::linear_allocator_resource( const mclo::span<std::byte> buffer,
														  std::pmr::memory_resource& upstream ) noexcept
		: m_buffer{ buffer }
		, m_current( buffer.data() )
		, m_upstream( &upstream )
	{
	}

	linear_allocator_resource::linear_allocator_resource( const mclo::span<std::byte> buffer ) noexcept
		: linear_allocator_resource( buffer, *std::pmr::get_default_resource() )
	{
	}

	linear_allocator_resource::~linear_allocator_resource()
	{
		release();
	}

	void* linear_allocator_resource::do_allocate( const std::size_t size, const std::size_t alignment )
	{
		if ( !align( alignment, size, m_current, m_buffer.data() + m_buffer.size() ) )
		{
			add_buffer_node( scaled_increase( size, m_buffer.size() ) );
		}

		std::byte* const result = m_current;
		m_current += size;
		return result;
	}

	void linear_allocator_resource::release()
	{
		while ( m_allocated_buffers )
		{
			const auto [ size, next ] = *m_allocated_buffers;
			std::destroy_at( m_allocated_buffers );
			m_upstream->deallocate( reinterpret_cast<std::byte*>( m_allocated_buffers ),
									size + sizeof( buffer_header ),
									alignof( buffer_header ) );
			m_allocated_buffers = next;
		}
	}

	void linear_allocator_resource::reset() noexcept
	{
		m_current = m_buffer.data();
	}

	void linear_allocator_resource::reset_consolidate() noexcept
	{
		if ( !m_allocated_buffers || !m_allocated_buffers->m_previous )
		{
			return;
		}
		const std::size_t last_size = m_buffer.size();
		release();
		add_buffer_node( last_size );
	}

	void linear_allocator_resource::add_buffer_node( const std::size_t size )
	{
		auto ptr =
			static_cast<std::byte*>( m_upstream->allocate( size + sizeof( buffer_header ), alignof( buffer_header ) ) );
		m_allocated_buffers = std::construct_at( reinterpret_cast<buffer_header*>( ptr ), size, m_allocated_buffers );
		m_buffer = { ptr + sizeof( buffer_header ), size };
		m_current = m_buffer.data();
	}

	[[nodiscard]] bool linear_allocator_resource::do_is_equal( const std::pmr::memory_resource& other ) const noexcept
	{
		return this == &other;
	}
}
