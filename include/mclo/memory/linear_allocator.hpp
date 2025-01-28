#pragma once

#include <memory>

#include "mclo/container/span.hpp"
#include "mclo/memory/alloca.hpp"

namespace mclo
{
	class linear_allocator_resource
	{
	public:
		explicit linear_allocator_resource( const mclo::span<std::byte> buffer ) noexcept;

		linear_allocator_resource( const linear_allocator_resource& ) = delete;
		linear_allocator_resource& operator=( const linear_allocator_resource& ) = delete;

		[[nodiscard]] std::byte* allocate( const std::size_t size, const std::size_t alignment ) noexcept;

		void reset() noexcept;

		[[nodiscard]] bool operator==( const linear_allocator_resource& other ) const noexcept
		{
			return m_buffer.data() == other.m_buffer.data() && m_buffer.size() == other.m_buffer.size();
		}

	private:
		mclo::span<std::byte> m_buffer;
		std::byte* m_current = nullptr;
	};

	namespace pmr
	{
		class linear_allocator_resource : public std::pmr::memory_resource, private mclo::linear_allocator_resource
		{
			using base = mclo::linear_allocator_resource;

		public:
			using base::base;
			using base::reset;

		private:
			void* do_allocate( const std::size_t size, const std::size_t align ) override
			{
				return base::allocate( size, align );
			}

			void do_deallocate( void* const, const std::size_t, const std::size_t ) override
			{
			}

			bool do_is_equal( const std::pmr::memory_resource& other ) const noexcept override
			{
				const auto ptr = dynamic_cast<const linear_allocator_resource*>( &other );
				if ( !ptr )
				{
					return false;
				}
				return static_cast<const base&>( *this ) == static_cast<const base&>( *ptr );
			}
		};
	}

	namespace detail
	{
		struct owned_byte_buffer
		{
			std::unique_ptr<std::byte[]> m_buffer;
		};
	}

	class heap_linear_allocator_resource : private detail::owned_byte_buffer, public linear_allocator_resource
	{
	public:
		explicit heap_linear_allocator_resource( const std::size_t size )
			: detail::owned_byte_buffer{ std::make_unique<std::byte[]>( size ) }
			, linear_allocator_resource( mclo::span( detail::owned_byte_buffer::m_buffer.get(), size ) )
		{
		}
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
	class linear_allocator
	{
		template <typename U>
		friend class linear_allocator;

	public:
		using value_type = T;
		using is_always_equal = std::false_type;
		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;

		linear_allocator( linear_allocator_resource& resource ) noexcept
			: m_resource( resource )
		{
		}

		template <typename U>
		linear_allocator( const linear_allocator<U>& other ) noexcept
			: m_resource( other.m_resource )
		{
		}

		template <typename U>
		linear_allocator& operator=( const linear_allocator<U>& other ) noexcept
		{
			m_resource = other.m_resource;
			return *this;
		}

		[[nodiscard]] bool operator==( const linear_allocator& other ) const noexcept
		{
			return m_resource.get() == other.m_resource.get();
		}

		[[nodiscard]] T* allocate( const std::size_t n ) noexcept
		{
			return reinterpret_cast<T*>( m_resource.get().allocate( sizeof( T ) * n, alignof( T ) ) );
		}

		void deallocate( T*, std::size_t ) noexcept
		{
			// Do nothing
		}

		void reset() noexcept
		{
			m_resource.get().reset();
		}

	private:
		std::reference_wrapper<linear_allocator_resource> m_resource;
	};
}
