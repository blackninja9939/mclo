#pragma once

#include <memory_resource>
#include <memory>

#include "mclo/container/span.hpp"
#include "mclo/memory/alloca.hpp"

namespace mclo
{
	class linear_allocator_resource : public std::pmr::memory_resource
	{
	private:
		struct buffer_header
		{
			buffer_header( const std::size_t size, buffer_header* const previous ) noexcept
				: m_size( size )
				, m_previous( previous )
			{
			}

			std::size_t m_size = 0;
			buffer_header* m_previous = nullptr;
		};

	public:
		explicit linear_allocator_resource( const std::size_t initial_size );
		linear_allocator_resource( const std::size_t initial_size, std::pmr::memory_resource& upstream );

		explicit linear_allocator_resource( const mclo::span<std::byte> buffer ) noexcept;
		linear_allocator_resource( const mclo::span<std::byte> buffer, std::pmr::memory_resource& upstream ) noexcept;

		~linear_allocator_resource();

		linear_allocator_resource( const linear_allocator_resource& ) = delete;
		linear_allocator_resource& operator=( const linear_allocator_resource& ) = delete;

		void release();

		void reset() noexcept;
		void reset_consolidate();
		
		[[nodiscard]] std::pmr::memory_resource* upstream_resource() const noexcept
		{
			return m_upstream;
		}

	protected:
		[[nodiscard]] void* do_allocate( const std::size_t size, const std::size_t alignment ) override;

		void do_deallocate( void*, const std::size_t, const std::size_t ) override
		{
		}

		[[nodiscard]] bool do_is_equal( const std::pmr::memory_resource& other ) const noexcept override;

	private:
		void add_buffer_node( const std::size_t size );

		std::pmr::memory_resource* m_upstream = nullptr;
		mclo::span<std::byte> m_buffer;
		std::byte* m_current = nullptr;
		buffer_header* m_allocated_buffers = nullptr;
	};

#define MCLO_ALLOCA_LINEAR_ALLOCATOR( NAME, TYPE, AMOUNT )                                                             \
	mclo::linear_allocator_resource NAME(                                                                              \
		{ reinterpret_cast<std::byte*>( MCLO_ALLOCA_TYPED( TYPE, AMOUNT ) ), sizeof( TYPE ) * AMOUNT } )

	template <std::size_t Bytes, std::size_t Align = alignof( std::max_align_t )>
	class inline_linear_allocator_resource : public linear_allocator_resource
	{
	public:
		inline_linear_allocator_resource() noexcept
			: linear_allocator_resource( m_buffer )
		{
		}
		explicit inline_linear_allocator_resource( std::pmr::memory_resource& upstream ) noexcept
			: linear_allocator_resource( m_buffer, upstream )
		{
		}

	private:
		alignas( Align ) std::byte m_buffer[ Bytes ];
	};

	template <typename T, std::size_t Num>
	using typed_inline_linear_allocator_resource = inline_linear_allocator_resource<sizeof( T ) * Num, alignof( T )>;
}
