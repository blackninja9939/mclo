#pragma once

#include <cstddef>
#include <new>

namespace mclo
{
	class new_delete_memory_resource
	{
	public:
		[[nodiscard]] std::byte* allocate( const std::size_t size, const std::size_t alignment )
		{
			return static_cast<std::byte*>( operator new( size, std::align_val_t{ alignment } ) );
		}

		void deallocate( std::byte* const ptr, const std::size_t size, const std::size_t alignment ) noexcept
		{
			operator delete( ptr, size, std::align_val_t{ alignment } );
		}

		[[nodiscard]] bool operator==( const new_delete_memory_resource& ) const noexcept
		{
			return true;
		}

		static new_delete_memory_resource& instance() noexcept
		{
			static new_delete_memory_resource obj;
			return obj;
		}
	};
}
