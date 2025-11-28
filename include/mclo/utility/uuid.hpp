#pragma once

#include <array>
#include <cstddef>
#include <string>

#include <mclo/debug/assert.hpp>

namespace mclo
{
	namespace detail
	{
		constexpr std::byte hex_char_to_byte( const char c ) MCLO_NOEXCEPT_TESTS
		{
			if ( c >= '0' && c <= '9' )
			{
				return static_cast<std::byte>( c - '0' );
			}
			else if ( c >= 'a' && c <= 'f' )
			{
				return static_cast<std::byte>( c - 'a' + 10 );
			}
			else if ( c >= 'A' && c <= 'F' )
			{
				return static_cast<std::byte>( c - 'A' + 10 );
			}
			else
			{
				PANIC( "Invalid hex character in UUID string", c );
			}
		}

		constexpr std::byte parse_byte_from_hex( const char high, const char low ) MCLO_NOEXCEPT_TESTS
		{
			return ( hex_char_to_byte( high ) << 4 ) | hex_char_to_byte( low );
		}

		constexpr std::array<std::byte, 16> parse_uuid_from_string( const std::string_view str ) MCLO_NOEXCEPT_TESTS
		{
			ASSERT( str.size() == 36, "Invalid UUID string length", str );
			ASSERT( str[ 8 ] == '-' && str[ 13 ] == '-' && str[ 18 ] == '-' && str[ 23 ] == '-',
					"Invalid UUID string format",
					str );

			std::array<std::byte, 16> bytes{};
			auto out = bytes.begin();

			auto first = str.begin(), last = str.end();
			while ( first != last )
			{
				const char c = *first++;
				if ( c == '-' )
				{
					continue;
				}
				*out++ = parse_byte_from_hex( c, *first++ );
			}

			return bytes;
		}
	}

	struct uuid
	{
		constexpr uuid() noexcept = default;

		constexpr uuid( const std::array<std::byte, 16>& bytes ) noexcept
			: bytes( bytes )
		{
		}

		// clang-format off
		constexpr uuid( const std::string_view str ) MCLO_NOEXCEPT_TESTS 
			: uuid( detail::parse_uuid_from_string( str ) )
		{
		}
		// clang-format on

		std::array<std::byte, 16> bytes{};

		[[nodiscard]] constexpr auto operator<=>( const uuid& other ) const noexcept = default;

		[[nodiscard]] std::string to_string() const;

		[[nodiscard]] static uuid generate();
	};

	std::string format_as( const uuid& u );
}
