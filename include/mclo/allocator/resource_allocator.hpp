#pragma once

#include "mclo/allocator/memory_resource.hpp"

#include <type_traits>
#include <utility>

namespace mclo
{
	template <memory_resource Resource, typename T>
	class resource_allocator
	{
		template <memory_resource R2, typename T2>
		friend class resource_allocator;

	public:
		using resource_type = Resource;
		using value_type = T;
		using is_always_equal = std::false_type;
		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;

		template <typename U>
		struct rebind
		{
			using other = resource_allocator<Resource, U>;
		};

		resource_allocator( resource_type& resource ) noexcept
			: m_resource( resource )
		{
		}

		template <typename U>
		resource_allocator( const resource_allocator<Resource, U>& other ) noexcept
			: m_resource( other.m_resource )
		{
		}

		template <typename U>
		resource_allocator& operator=( const resource_allocator<Resource, U>& other ) noexcept
		{
			m_resource = other.m_resource;
			return *this;
		}

		[[nodiscard]] bool operator==( const resource_allocator& other ) const noexcept
		{
			return m_resource.get() == other.m_resource.get();
		}

		[[nodiscard]] T* allocate( const std::size_t n )
		{
			return reinterpret_cast<T*>( m_resource.get().allocate( sizeof( T ) * n, alignof( T ) ) );
		}

		void deallocate( T* const ptr, const std::size_t n ) noexcept
		{
			m_resource.get().deallocate( reinterpret_cast<std::byte*>( ptr ), sizeof( T ) * n, alignof( T ) );
		}

		resource_type& resource() noexcept
		{
			return m_resource;
		}

	private:
		std::reference_wrapper<resource_type> m_resource;
	};
}
