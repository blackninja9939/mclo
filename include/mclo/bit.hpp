#pragma once

#include <bit>

#ifdef __cpp_lib_byteswap

namespace mclo
{
	using std::byteswap;
}

#else

#include "always_false.hpp"

#include <algorithm>
#include <array>
#include <concepts>

namespace mclo
{
	namespace detail
	{
		template <typename T>
		[[nodiscard]] constexpr T byteswap( const T value ) noexcept
		{
			auto bytes = std::bit_cast<std::array<std::byte, sizeof( T )>>( value );
			std::reverse( bytes.begin(), bytes.end() );
			return std::bit_cast<T>( bytes );
		}
	}

	template <std::integral T>
	[[nodiscard]] constexpr T byteswap( const T value ) noexcept
	{
		if constexpr ( sizeof( T ) == 1 )
		{
			return value;
		}
		else
		{
#ifdef _MSC_VER // On MSVC at run time use their intrinsics
			if ( std::is_constant_evaluated() )
			{
				return detail::byteswap( value );
			}
			else
			{
				if constexpr ( sizeof( T ) == 2 )
				{
					return static_cast<T>( _byteswap_ushort( static_cast<unsigned short>( value ) ) );
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
					return static_cast<T>( _byteswap_ulong( static_cast<unsigned long>( value ) ) );
				}
				else if constexpr ( sizeof( T ) == 8 )
				{
					return static_cast<T>( _byteswap_uint64( static_cast<unsigned long long>( value ) ) );
				}
				else
				{
					static_assert( mclo::always_false<T>, "Invalid integer size" );
				}
			}
#else           // Else always use naive implementation
			return detail::byteswap( value );
#endif
		}
	}
}
#endif
