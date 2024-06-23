#include <catch2/catch_test_macros.hpp>

#include <cinttypes>
#include <array>

#include "mclo/bit.hpp"

namespace
{
	constexpr std::uint32_t make_test_value()
	{
		std::uint32_t value = 0b101010;
		value <<= 16;
		value += 0b010101;
		return value;
	}
}

TEST_CASE( "bit_cast", "[bit]" )
{
	const std::uint32_t value = 0b1010100000000000010101;

	const auto bytes = mclo::bit_cast<std::array<std::byte, sizeof( std::uint32_t )>>( value );

	CHECK( bytes[ 0 ] == std::byte( 0b010101 ) );
	CHECK( bytes[ 1 ] == std::byte( 0 ) );
	CHECK( bytes[ 2 ] == std::byte( 0b101010 ) );
	CHECK( bytes[ 3 ] == std::byte( 0 ) );

	const auto round_trip = mclo::bit_cast<std::uint32_t>( bytes );
	CHECK( value == round_trip );

	const std::array modified_bytes{ bytes[ 0 ], std::byte( 0b1101111 ), bytes[ 2 ], bytes[ 3 ] };

	const auto modified_value = mclo::bit_cast<std::uint32_t>( modified_bytes );
	CHECK( modified_value == 0b1010100110111100010101 );
}

TEST_CASE( "constexpr bit_cast", "[bit]" )
{
	constexpr std::uint32_t value = 0b1010100000000000010101;

	constexpr auto bytes = mclo::bit_cast<std::array<std::byte, sizeof( std::uint32_t )>>( value );

	STATIC_CHECK( bytes[ 0 ] == std::byte( 0b010101 ) );
	STATIC_CHECK( bytes[ 1 ] == std::byte( 0 ) );
	STATIC_CHECK( bytes[ 2 ] == std::byte( 0b101010 ) );
	STATIC_CHECK( bytes[ 3 ] == std::byte( 0 ) );

	constexpr auto round_trip = mclo::bit_cast<std::uint32_t>( bytes );
	STATIC_CHECK( value == round_trip );

	constexpr std::array modified_bytes{ bytes[ 0 ], std::byte( 0b1101111 ), bytes[2], bytes[3] };

	constexpr auto modified_value = mclo::bit_cast<std::uint32_t>( modified_bytes );
	STATIC_CHECK( modified_value == 0b1010100110111100010101 );
}

TEST_CASE( "byteswap", "[bit]" )
{
	const std::uint32_t value = 0b00000000001010100000000000010101;

	const std::uint32_t swapped = mclo::byteswap( value );

	CHECK( swapped == 0b00010101000000000010101000000000 );
}

TEST_CASE( "constexpr byteswap", "[bit]" )
{
	constexpr std::uint32_t value = 0b00000000001010100000000000010101;

	constexpr std::uint32_t swapped = mclo::byteswap( value );

	STATIC_CHECK( swapped == 0b00010101000000000010101000000000 );
}

TEST_CASE( "countl_zero", "[bit]" )
{
	const int result = mclo::countl_zero( 0b10101001u );
	CHECK( result == 32 - 8 );
}

TEST_CASE( "constexpr countl_zero", "[bit]" )
{
	constexpr int result = mclo::countl_zero( 0b10101001u );
	STATIC_CHECK( result == 32 - 8 );
}

TEST_CASE( "countr_zero", "[bit]" )
{
	const int result = mclo::countr_zero( 0b11010000u );
	CHECK( result == 4 );
}

TEST_CASE( "constexpr countr_zero", "[bit]" )
{
	constexpr int result = mclo::countr_zero( 0b11010000u );
	STATIC_CHECK( result == 4 );
}

TEST_CASE( "rotl", "[bit]" )
{
	const int result = mclo::rotl( 208u, 2 );
	CHECK( result == 832 );
}

TEST_CASE( "constexpr rotl", "[bit]" )
{
	constexpr int result = mclo::rotl( 208u, 2 );
	STATIC_CHECK( result == 832 );
}

TEST_CASE( "rotr", "[bit]" )
{
	const int result = mclo::rotr( 208u, 2 );
	CHECK( result == 52 );
}

TEST_CASE( "constexpr rotr", "[bit]" )
{
	constexpr int result = mclo::rotr( 208u, 2 );
	STATIC_CHECK( result == 52 );
}
