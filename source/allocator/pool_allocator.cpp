#include "mclo/allocator/pool_allocator.hpp"

#include "mclo/debug/assert.hpp"
#include "mclo/numeric/align.hpp"
#include "mclo/numeric/pow2.hpp"

#include <new>
#include <utility>

namespace mclo
{
	memory_pool::memory_pool( const std::size_t chunk_count,
							  const std::size_t chunk_size,
							  const std::size_t chunk_alignment )
		: m_chunk_alignment( std::max( chunk_alignment, alignof( free_list_node ) ) )
		, m_chunk_size( mclo::align_up( std::max( chunk_size, sizeof( free_list_node ) ), m_chunk_alignment ) )
		, m_data( static_cast<std::byte*>(
			  ::operator new( m_chunk_size* chunk_count, std::align_val_t( m_chunk_alignment ) ) ) )
	{
		DEBUG_ASSERT( mclo::is_pow2( m_chunk_alignment ), "Chunk alignment must be a power of 2" );
		DEBUG_ASSERT( chunk_size % m_chunk_alignment == 0, "Chunk size must be a multiple of chunk alignment" );

		std::byte* ptr = m_data;
		for ( std::size_t i = 0; i < chunk_count; ++i )
		{
			m_free_list.push_front( *std::construct_at( reinterpret_cast<free_list_node*>( ptr ) ) );
			ptr += m_chunk_size;
		}
	}

	memory_pool::~memory_pool()
	{
		m_free_list.clear();
		::operator delete( m_data, std::align_val_t( m_chunk_alignment ) );
	}

	memory_pool::memory_pool( memory_pool&& other ) noexcept
		: m_chunk_alignment( std::exchange( other.m_chunk_alignment, 0 ) )
		, m_chunk_size( std::exchange( other.m_chunk_size, 0 ) )
		, m_data( std::exchange( other.m_data, nullptr ) )
		, m_free_list( std::move( other.m_free_list ) )
	{
	}

	void* memory_pool::allocate( const std::size_t size,
								 const std::size_t alignment /*= alignof( std::max_align_t ) */ )
	{
		DEBUG_ASSERT( size <= m_chunk_size, "Requested size exceeds chunk size" );
		DEBUG_ASSERT( alignment <= m_chunk_alignment, "Requested alignment exceeds chunk alignment" );
		free_list_node* const node = m_free_list.pop_front();

		if ( !node )
		{
			throw std::bad_alloc();
		}

		std::destroy_at( node );
		return static_cast<void*>( node );
	}

	void memory_pool::deallocate( void* const ptr,
								  const std::size_t size,
								  const std::size_t alignment /*= alignof( std::max_align_t ) */ ) noexcept
	{
		DEBUG_ASSERT( size <= m_chunk_size, "Deallocated size exceeds chunk size" );
		DEBUG_ASSERT( alignment <= m_chunk_alignment, "Requested alignment exceeds chunk alignment" );

		// Reconstruct the node and push it back to the free list
		// Construct at ensures pointer provenance without needing std::launder
		auto node = std::construct_at( static_cast<free_list_node*>( ptr ) );
		m_free_list.push_front( *node );
	}
}
