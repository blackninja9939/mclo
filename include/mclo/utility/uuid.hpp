#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <optional>
#include <string>

#include <mclo/debug/assert.hpp>

namespace mclo
{
	namespace detail
	{
		constexpr bool is_hex_char( const char c ) noexcept
		{
			return ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' );
		}

		constexpr std::byte hex_char_to_byte( const char c ) noexcept
		{
			if ( c >= '0' && c <= '9' )
			{
				return static_cast<std::byte>( c - '0' );
			}
			else if ( c >= 'a' && c <= 'f' )
			{
				return static_cast<std::byte>( c - 'a' + 10 );
			}
			else
			{
				return static_cast<std::byte>( c - 'A' + 10 );
			}
		}

		constexpr std::byte parse_byte_from_hex( const char high, const char low ) noexcept
		{
			return ( hex_char_to_byte( high ) << 4 ) | hex_char_to_byte( low );
		}

		constexpr std::optional<std::array<std::byte, 16>> try_parse_uuid_from_string(
			const std::string_view str ) noexcept
		{
			if ( str.size() != 36 )
			{
				return std::nullopt;
			}
			if ( str[ 8 ] != '-' || str[ 13 ] != '-' || str[ 18 ] != '-' || str[ 23 ] != '-' )
			{
				return std::nullopt;
			}

			std::array<std::byte, 16> bytes{};
			auto out = bytes.begin();

			auto first = str.begin();
			const auto last = str.end();
			while ( first != last )
			{
				const char c = *first++;
				if ( c == '-' )
				{
					continue;
				}
				const char low = *first++;
				if ( !is_hex_char( c ) || !is_hex_char( low ) )
				{
					return std::nullopt;
				}
				*out++ = parse_byte_from_hex( c, low );
			}

			return bytes;
		}

		constexpr std::array<std::byte, 16> parse_uuid_from_string( const std::string_view str ) MCLO_NOEXCEPT_TESTS
		{
			auto result = try_parse_uuid_from_string( str );
			ASSERT( result.has_value(), "Invalid UUID string", str );
			return *result;
		}
	}

	/// @brief A universally unique identifier (UUID) following RFC 4122.
	/// @details The struct itself is version-agnostic and can store any UUID. Can be constructed from
	/// raw bytes, parsed from a string in the standard "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" format,
	/// or generated randomly as a version 4 UUID via the generate overloads.
	struct uuid
	{
		/// @brief Constructs a nil UUID with all bytes set to zero.
		constexpr uuid() noexcept = default;

		/// @brief Constructs a UUID from a raw 16-byte array.
		/// @param bytes The 16 bytes of the UUID in network byte order.
		constexpr uuid( const std::array<std::byte, 16>& bytes ) noexcept
			: bytes( bytes )
		{
		}

		/// @brief Parses a UUID from its string representation.
		/// @param str A 36-character string in the format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
		/// @pre @p str must be a valid UUID string.
		// clang-format off
		constexpr uuid( const std::string_view str ) MCLO_NOEXCEPT_TESTS
			: uuid( detail::parse_uuid_from_string( str ) )
		{
		}
		// clang-format on

		/// @brief The 16 bytes of the UUID.
		std::array<std::byte, 16> bytes{};

		/// @brief Defaulted three-way comparison operator.
		[[nodiscard]] constexpr auto operator<=>( const uuid& other ) const noexcept = default;

		/// @brief Converts the UUID to its string representation.
		/// @return A 36-character string in the format "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
		[[nodiscard]] std::string to_string() const;

		/// @brief Attempts to parse a UUID from a string.
		/// @param str The string to parse.
		/// @return The parsed UUID, or @c std::nullopt if the string is not a valid UUID.
		[[nodiscard]] static constexpr std::optional<uuid> try_parse( const std::string_view str ) noexcept
		{
			auto result = detail::try_parse_uuid_from_string( str );
			if ( !result )
			{
				return std::nullopt;
			}
			return uuid{ *result };
		}

		/// @brief Generates a random version 4 UUID using @c std::random_device.
		/// @return A new UUID with version 4 and RFC 4122 variant bits set.
		[[nodiscard]] static uuid generate();

		/// @brief Generates a random version 4 UUID using a caller-provided random engine.
		/// @tparam Generator A type satisfying the uniform random bit generator requirements.
		/// @param generator The random engine to draw bits from.
		/// @return A new UUID with version 4 and RFC 4122 variant bits set.
		template <typename Generator>
		[[nodiscard]] static uuid generate( Generator& generator )
		{
			std::array<unsigned int, 4> bytes;
			for ( auto& byte : bytes )
			{
				byte = generator();
			}

			uuid new_uuid{ std::bit_cast<std::array<std::byte, 16>>( bytes ) };
			// Set the version to 4 aka random UUID
			new_uuid.bytes[ 6 ] = ( new_uuid.bytes[ 6 ] & std::byte{ 0x0F } ) | std::byte{ 0x40 };
			// Set the variant to RFC 4122
			new_uuid.bytes[ 8 ] = ( new_uuid.bytes[ 8 ] & std::byte{ 0x3F } ) | std::byte{ 0x80 };
			return new_uuid;
		}
	};

	/// @brief Formats a UUID as its string representation for use with fmtlib/std::format.
	/// @param u The UUID to format.
	/// @return The string representation of @p u.
	std::string format_as( const uuid& u );
}
