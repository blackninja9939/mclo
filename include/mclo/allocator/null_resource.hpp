#pragma once

#include "mclo/debug/assert.hpp"

#include <cstddef>
#include <new>

namespace mclo
{
	class null_memory_resource
	{
	public:
		[[noreturn]] std::byte* allocate( const std::size_t, const std::size_t )
		{
			throw std::bad_alloc();
		}

		void deallocate( std::byte* const, const std::size_t, const std::size_t ) noexcept
		{
		}

		[[nodiscard]] bool operator==( const null_memory_resource& ) const noexcept
		{
			return true;
		}

		static null_memory_resource& instance() noexcept
		{
			static null_memory_resource obj;
			return obj;
		}
	};
}
