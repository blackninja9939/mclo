#include <catch2/catch_test_macros.hpp>

#include <cinttypes>

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
