#pragma once

#include <concepts>
#include <cstddef>
#include <memory_resource>
#include <type_traits>

namespace mclo
{
	template <typename T>
	concept memory_resource = requires( T& r, const std::size_t n, std::byte* const ptr ) {
		{
			r.allocate( n, n )
		} -> std::same_as<std::byte*>;
		{
			r.deallocate( ptr, n, n )
		};
		requires std::equality_comparable<T>;
	};

	template <memory_resource T>
	class pmr_resource_adapter : public std::pmr::memory_resource
	{
	public:
		template <typename... Args>
		explicit pmr_resource_adapter( Args&&... args ) noexcept( std::is_nothrow_constructible_v<T, Args...> )
			: m_resource( std::forward<Args>( args )... )
		{
		}

	private:
		void* do_allocate( const std::size_t size, const std::size_t align ) override
		{
			return m_resource.allocate( size, align );
		}

		void do_deallocate( void* const ptr, const std::size_t size, const std::size_t align ) override
		{
			m_resource.deallocate( reinterpret_cast<std::byte*>( ptr ), size, align );
		}

		bool do_is_equal( const std::pmr::memory_resource& other ) const noexcept override
		{
			const auto ptr = dynamic_cast<const pmr_resource_adapter*>( &other );
			if ( !ptr )
			{
				return false;
			}
			return m_resource == ptr->m_resource;
		}

		T m_resource;
	};
}
