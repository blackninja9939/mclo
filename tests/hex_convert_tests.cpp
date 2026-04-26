#include <catch2/catch_test_macros.hpp>

#include "mclo/string/hex_convert.hpp"

#include "assert_macros.hpp"

#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include <array>
#include <cctype>
#include <charconv>
#include <limits>
#include <numeric>
#include <string>

using namespace Catch::Matchers;

namespace
{
	constexpr std::size_t byte_count = static_cast<std::size_t>( std::numeric_limits<std::uint8_t>::max() ) + 1;
	constexpr auto hex_digit_count = std::uint8_t( 16 );
}

// -- is_hex (single char) --

TEST_CASE( "all 8 bit values, is hex digit, returns same as std::isxdigit", "[hex_convert]" )
{
	for ( std::size_t i = 0; i < byte_count; ++i )
	{
		const bool std_result = std::isxdigit( static_cast<int>( i ) );

		const bool mclo_result = mclo::is_hex( static_cast<char>( i ) );

		CHECK( std_result == mclo_result );
	}
}

// -- from_hex (single char) --

TEST_CASE( "all hex digit characters, from_hex returns same as std::from_chars", "[hex_convert]" )
{
	for ( const char c : { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a',
						   'b', 'c', 'd', 'e', 'f', 'A', 'B', 'C', 'D', 'E', 'F' } )
	{
		std::uint8_t std_result = 0;
		const auto parseResult = std::from_chars( &c, &c + 1, std_result, 16 );
		REQUIRE( parseResult.ec == std::errc() );

		const std::uint8_t mclo_result = mclo::from_hex( c );

		CHECK( std_result == mclo_result );
	}
}

// -- to_hex (single value) --

TEST_CASE( "all hex digit values, to_hex, returns same as std::to_chars", "[hex_convert]" )
{
	for ( std::uint8_t i = 0; i < hex_digit_count; ++i )
	{
		char std_result;
		const auto writeResult = std::to_chars( &std_result, &std_result + 1, i, 16 );
		REQUIRE( writeResult.ec == std::errc() );

		const char mclo_result = mclo::to_hex( i );

		CHECK( std_result == mclo_result );
	}
}

// -- to_hex_upper (single value) --

TEST_CASE( "all hex digit values, to_hex_upper returns same as std::toupper(std::to_chars)", "[hex_convert]" )
{
	for ( std::uint8_t i = 0; i < hex_digit_count; ++i )
	{
		char std_result;
		const auto writeResult = std::to_chars( &std_result, &std_result + 1, i, 16 );
		REQUIRE( writeResult.ec == std::errc() );
		std_result = static_cast<char>( std::toupper( std_result ) );

		const char mclo_result = mclo::to_hex_upper( i );

		CHECK( std_result == mclo_result );
	}
}

// -- to_hex (bulk) --

TEST_CASE( "empty input, to_hex, returns empty string", "[hex_convert]" )
{
	const mclo::span<const std::uint8_t> input;

	const std::string result = mclo::to_hex( input );

	CHECK( result.empty() );
}

TEST_CASE( "single byte, to_hex, returns two hex characters", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 1> input = { 0xAB };

	const std::string result = mclo::to_hex( input );

	CHECK( result == "ab" );
}

TEST_CASE( "multiple bytes, to_hex, returns correct hex string", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 4> input = { 0xDE, 0xAD, 0xBE, 0xEF };

	const std::string result = mclo::to_hex( input );

	CHECK( result == "deadbeef" );
}

TEST_CASE( "bytes with leading zeros, to_hex, preserves leading zeros", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 3> input = { 0x00, 0x01, 0x0A };

	const std::string result = mclo::to_hex( input );

	CHECK( result == "00010a" );
}

TEST_CASE( "multiple bytes, to_hex span overload, writes correct hex to buffer", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 2> input = { 0xCA, 0xFE };
	std::array<char, 4> output = {};

	mclo::to_hex( input, output );

	CHECK( std::string_view( output.data(), output.size() ) == "cafe" );
}

TEST_CASE( "all byte values, to_hex span, matches per-byte to_hex", "[hex_convert]" )
{
	for ( std::size_t i = 0; i < byte_count; ++i )
	{
		const auto byte = static_cast<std::uint8_t>( i );
		const std::string bulk = mclo::to_hex( mclo::span( &byte, 1 ) );

		const char expected_hi = mclo::to_hex( byte >> 4 );
		const char expected_lo = mclo::to_hex( byte );

		CHECK( bulk[ 0 ] == expected_hi );
		CHECK( bulk[ 1 ] == expected_lo );
	}
}

TEST_CASE( "to_hex span, output buffer too small, asserts", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 2> input = { 0xAB, 0xCD };
	std::array<char, 3> output = {};

	CHECK_ASSERTS( mclo::to_hex( input, output ), "Output buffer too small for hex conversion" );
}

// -- to_hex_upper (bulk) --

TEST_CASE( "empty input, to_hex_upper, returns empty string", "[hex_convert]" )
{
	const mclo::span<const std::uint8_t> input;

	const std::string result = mclo::to_hex_upper( input );

	CHECK( result.empty() );
}

TEST_CASE( "multiple bytes, to_hex_upper, returns uppercase hex string", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 4> input = { 0xDE, 0xAD, 0xBE, 0xEF };

	const std::string result = mclo::to_hex_upper( input );

	CHECK( result == "DEADBEEF" );
}

TEST_CASE( "multiple bytes, to_hex_upper span overload, writes correct hex to buffer", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 2> input = { 0xCA, 0xFE };
	std::array<char, 4> output = {};

	mclo::to_hex_upper( input, output );

	CHECK( std::string_view( output.data(), output.size() ) == "CAFE" );
}

TEST_CASE( "to_hex_upper span, output buffer too small, asserts", "[hex_convert]" )
{
	constexpr std::array<std::uint8_t, 2> input = { 0xAB, 0xCD };
	std::array<char, 3> output = {};

	CHECK_ASSERTS( mclo::to_hex_upper( input, output ), "Output buffer too small for hex conversion" );
}

// -- from_hex (bulk) --

TEST_CASE( "empty input, from_hex span, writes nothing", "[hex_convert]" )
{
	std::array<std::uint8_t, 1> output = { 0xFF };

	mclo::from_hex( "", output );

	CHECK( output[ 0 ] == 0xFF );
}

TEST_CASE( "single byte, from_hex span, decodes correctly", "[hex_convert]" )
{
	std::array<std::uint8_t, 1> output = {};

	mclo::from_hex( "ab", output );

	CHECK( output[ 0 ] == 0xAB );
}

TEST_CASE( "multiple bytes, from_hex span, decodes correctly", "[hex_convert]" )
{
	std::array<std::uint8_t, 4> output = {};

	mclo::from_hex( "deadbeef", output );

	CHECK_THAT( output, RangeEquals( std::array<std::uint8_t, 4>{ 0xDE, 0xAD, 0xBE, 0xEF } ) );
}

TEST_CASE( "uppercase input, from_hex span, decodes correctly", "[hex_convert]" )
{
	std::array<std::uint8_t, 4> output = {};

	mclo::from_hex( "DEADBEEF", output );

	CHECK_THAT( output, RangeEquals( std::array<std::uint8_t, 4>{ 0xDE, 0xAD, 0xBE, 0xEF } ) );
}

TEST_CASE( "mixed case input, from_hex span, decodes correctly", "[hex_convert]" )
{
	std::array<std::uint8_t, 4> output = {};

	mclo::from_hex( "DeAdBeEf", output );

	CHECK_THAT( output, RangeEquals( std::array<std::uint8_t, 4>{ 0xDE, 0xAD, 0xBE, 0xEF } ) );
}

TEST_CASE( "from_hex span roundtrips with to_hex for all byte values", "[hex_convert]" )
{
	std::array<std::uint8_t, byte_count> original;
	std::iota( original.begin(), original.end(), std::uint8_t( 0 ) );

	const std::string hex = mclo::to_hex( original );

	std::array<std::uint8_t, byte_count> decoded = {};
	mclo::from_hex( hex, decoded );

	CHECK_THAT( decoded, RangeEquals( original ) );
}

TEST_CASE( "from_hex span, output buffer too small, asserts", "[hex_convert]" )
{
	std::array<std::uint8_t, 1> output = {};

	CHECK_ASSERTS( mclo::from_hex( "aabb", output ), "Output buffer too small for hex decoding" );
}

TEST_CASE( "from_hex span, odd length input, asserts", "[hex_convert]" )
{
	std::array<std::uint8_t, 2> output = {};

	CHECK_ASSERTS( mclo::from_hex( "abc", output ), "Hex string must have an even length" );
}
