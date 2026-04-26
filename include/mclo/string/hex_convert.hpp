#pragma once

#include "mclo/container/span.hpp"
#include "mclo/debug/assert.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace mclo
{
	/// @brief Checks if a character is a valid hexadecimal digit (0-9, a-f, A-F).
	/// @param c The character to check.
	/// @return @c true if the character is a hexadecimal digit, @c false otherwise.
	[[nodiscard]] constexpr bool is_hex( const char c ) noexcept
	{
		const std::uint32_t x = c;
		const std::uint32_t a = ( x - '0' ) <= 9;
		const std::uint32_t b = ( ( x | 32 ) - 'a' ) <= 5;
		return a | b;
	}

	/// @brief Converts a hexadecimal character to its corresponding 4-bit integer value.
	/// @param c The hexadecimal character to convert (0-9, a-f, A-F).
	/// @return The integer value of the hexadecimal character (0-15).
	/// @pre @p c must be a valid hexadecimal digit. The behavior is undefined for invalid input.
	[[nodiscard]] constexpr std::uint8_t from_hex( const char c ) noexcept
	{
		return ( ( c & 0xf ) + ( c >> 6 ) * 9 );
	}

	/// @brief Converts a 4-bit value to its corresponding lowercase hexadecimal character.
	/// @param value The 4-bit value to convert (0-15).
	/// @return The hexadecimal character corresponding to the input value ('0'-'9', 'a'-'f').
	/// @pre @p value must be in the range 0-15. The behavior is undefined for values outside this range.
	[[nodiscard]] constexpr char to_hex( const std::uint8_t value ) noexcept
	{
		return "0123456789abcdef"[ value & 0x0F ];
	}

	/// @brief Converts a 4-bit value to its corresponding uppercase hexadecimal character.
	/// @param value The 4-bit value to convert (0-15).
	/// @return The uppercase hexadecimal character corresponding to the input value ('0'-'9', 'A'-'F').
	/// @pre @p value must be in the range 0-15. The behavior is undefined for values outside this range.
	[[nodiscard]] constexpr char to_hex_upper( const std::uint8_t value ) noexcept
	{
		return "0123456789ABCDEF"[ value & 0x0F ];
	}

	/// @brief Decodes a hex string into bytes.
	/// @param input The hex string to decode. Each pair of characters is decoded into one byte.
	/// @param output The destination buffer for the decoded bytes.
	/// @pre @p input must contain only valid hexadecimal characters and have an even length.
	/// @pre @p output must have a size of at least @p input size / 2.
	constexpr void from_hex( const std::string_view input, const mclo::span<std::uint8_t> output ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( input.size() % 2 == 0, "Hex string must have an even length" );
		DEBUG_ASSERT( output.size() >= input.size() / 2, "Output buffer too small for hex decoding" );
		for ( std::size_t i = 0; i < input.size(); i += 2 )
		{
			output[ i / 2 ] = static_cast<std::uint8_t>( ( from_hex( input[ i ] ) << 4 ) | from_hex( input[ i + 1 ] ) );
		}
	}

	/// @brief Converts a span of bytes to their lowercase hexadecimal representation.
	/// @param input The bytes to convert.
	/// @param output The destination buffer for the hexadecimal characters.
	/// @pre @p output must have a size of at least @p input size * 2.
	constexpr void to_hex( const mclo::span<const std::uint8_t> input,
						   const mclo::span<char> output ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( output.size() >= input.size() * 2, "Output buffer too small for hex conversion" );
		for ( std::size_t i = 0; i < input.size(); ++i )
		{
			output[ i * 2 ] = to_hex( input[ i ] >> 4 );
			output[ i * 2 + 1 ] = to_hex( input[ i ] );
		}
	}

	/// @brief Converts a span of bytes to their uppercase hexadecimal representation.
	/// @param input The bytes to convert.
	/// @param output The destination buffer for the hexadecimal characters.
	/// @pre @p output must have a size of at least @p input size * 2.
	constexpr void to_hex_upper( const mclo::span<const std::uint8_t> input,
								 const mclo::span<char> output ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( output.size() >= input.size() * 2, "Output buffer too small for hex conversion" );
		for ( std::size_t i = 0; i < input.size(); ++i )
		{
			output[ i * 2 ] = to_hex_upper( input[ i ] >> 4 );
			output[ i * 2 + 1 ] = to_hex_upper( input[ i ] );
		}
	}

	/// @brief Converts a span of bytes to a lowercase hexadecimal string.
	/// @param input The bytes to convert.
	/// @return A string containing the lowercase hexadecimal representation of the input bytes.
	[[nodiscard]] inline std::string to_hex( const mclo::span<const std::uint8_t> input )
	{
		std::string result( input.size() * 2, '\0' );
		to_hex( input, result );
		return result;
	}

	/// @brief Converts a span of bytes to an uppercase hexadecimal string.
	/// @param input The bytes to convert.
	/// @return A string containing the uppercase hexadecimal representation of the input bytes.
	[[nodiscard]] inline std::string to_hex_upper( const mclo::span<const std::uint8_t> input )
	{
		std::string result( input.size() * 2, '\0' );
		to_hex_upper( input, result );
		return result;
	}
}
