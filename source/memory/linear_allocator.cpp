#include "mclo/memory/linear_allocator.hpp"

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
}

namespace mclo
{
	linear_allocator_resource::linear_allocator_resource( const mclo::span<std::byte> buffer ) noexcept
		: m_buffer( buffer )
		, m_current( buffer.data() )
	{
	}

	std::byte* linear_allocator_resource::allocate( const std::size_t size, const std::size_t alignment ) noexcept
	{
		if ( align( alignment, size, m_current, m_buffer.data() + m_buffer.size() ) )
		{
			std::byte* const result = m_current;
			m_current += size;
			return result;
		}
		return nullptr;
	}

	void linear_allocator_resource::reset() noexcept
	{
		m_current = m_buffer.data();
	}
}
