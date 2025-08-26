#include <catch2/catch_test_macros.hpp>

#include "consteval_check.hpp"

#include <cinttypes>

#include "mclo/numeric/bit.hpp"

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

TEST_CASE( "byteswap", "[bit]" )
{
	constexpr std::uint32_t value = 0b00000000001010100000000000010101;
	CONSTEVAL_CHECK( mclo::byteswap( value ) == 0b00010101000000000010101000000000 );
}

TEST_CASE( "bit_reverse", "[bit]" )
{
	CONSTEVAL_CHECK( mclo::bit_reverse( 0b00000000001010100000000000010101u ) == 0b10101000000000000101010000000000 );
	CONSTEVAL_CHECK( mclo::bit_reverse( static_cast<std::uint8_t>( 0b01110010 ) ) == 0b01001110 );
}

TEST_CASE( "bit_repeat", "[bit]" )
{
	CONSTEVAL_CHECK( mclo::bit_repeat( 0b111101u, 5 ) == 0b01111011110111101111011110111101 );
	CONSTEVAL_CHECK( mclo::bit_repeat( 0xcu, 4 ) == 0xcccccccc );
}

TEST_CASE( "bit_compress", "[bit]" )
{
	CONSTEVAL_CHECK( mclo::bit_compress( 0b010011u, 0b011101u ) == 0b1001u );
	CONSTEVAL_CHECK( mclo::bit_compress( 0b010011u, 0b111111u ) == 0b010011u );
	CONSTEVAL_CHECK( mclo::bit_compress( 0b010011u, 0b0u ) == 0b0u );
}

TEST_CASE( "bit_expand", "[bit]" )
{
	CONSTEVAL_CHECK( mclo::bit_expand( 0b010011u, 0b011101u ) == 0b000101u );
	CONSTEVAL_CHECK( mclo::bit_expand( 0b010011u, 0b0u ) == 0b0u );
	CONSTEVAL_CHECK( mclo::bit_expand( 0b010011u, 0b1111111u ) == 0b010011u );
}
