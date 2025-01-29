#pragma once

#include "mclo/allocator/memory_resource.hpp"

namespace mclo
{
	namespace detail
	{
		struct resource_functions
		{
			using allocate = std::byte* ( * )( void*, std::size_t, std::size_t );
			using deallocate = void ( * )( void*, std::byte*, std::size_t, std::size_t );

			allocate m_allocate;
			deallocate m_deallocate;
		};

		template <typename T>
		inline constexpr resource_functions resource_funcs = {
			.m_allocate = []( void* const alloc, const std::size_t size, const std::size_t align ) -> std::byte* {
				return static_cast<T*>( alloc )->allocate( size, align );
			},
			.m_deallocate =
				[]( void* const alloc, std::byte* ptr, const std::size_t size, const std::size_t align ) {
					return static_cast<T*>( alloc )->deallocate( ptr, size, align );
				} };
	}

	class upstream_resource
	{
	public:
		template <memory_resource T>
		explicit upstream_resource( T& upstream ) noexcept
			: m_upstream( &upstream )
			, m_functions( &detail::resource_funcs<T> )
		{
		}

		[[nodiscard]] std::byte* allocate( const std::size_t size, const std::size_t align )
		{
			return m_functions->m_allocate( m_upstream, size, align );
		}

		void deallocate( std::byte* const ptr, const std::size_t size, const std::size_t align ) noexcept
		{
			m_functions->m_deallocate( m_upstream, ptr, size, align );
		}

	private:
		void* m_upstream = nullptr;
		const detail::resource_functions* m_functions = nullptr;
	};
}
