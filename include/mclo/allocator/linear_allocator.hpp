#pragma once

#include <memory>

#include "mclo/allocator/resource_allocator.hpp"
#include "mclo/allocator/upstream_resource.hpp"
#include "mclo/container/span.hpp"
#include "mclo/memory/alloca.hpp"

namespace mclo
{
	class linear_allocator_resource
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
		linear_allocator_resource( const std::size_t initial_size, upstream_resource upstream );

		explicit linear_allocator_resource( const mclo::span<std::byte> buffer ) noexcept;
		linear_allocator_resource( const mclo::span<std::byte> buffer, upstream_resource upstream ) noexcept;

		~linear_allocator_resource();

		linear_allocator_resource( const linear_allocator_resource& ) = delete;
		linear_allocator_resource& operator=( const linear_allocator_resource& ) = delete;

		[[nodiscard]] std::byte* allocate( const std::size_t size, const std::size_t alignment ) noexcept;
		void deallocate( std::byte*, const std::size_t, const std::size_t ) noexcept
		{
		}

		void reset() noexcept;
		void reset_consolidate() noexcept;

		[[nodiscard]] bool operator==( const linear_allocator_resource& other ) const noexcept;

	private:
		void release();
		void add_buffer_node( const std::size_t size );

		upstream_resource m_upstream;
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

	private:
		alignas( Align ) std::byte m_buffer[ Bytes ];
	};

	template <typename T, std::size_t Num>
	using typed_inline_linear_allocator_resource = inline_linear_allocator_resource<sizeof( T ) * Num, alignof( T )>;

	template <typename T>
	using linear_allocator = resource_allocator<linear_allocator_resource, T>;
}
