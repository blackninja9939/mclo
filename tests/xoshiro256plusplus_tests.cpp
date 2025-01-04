#include <catch2/catch_test_macros.hpp>

#include "mclo/random/xoshiro256plusplus.hpp"

#include <random>

namespace
{
	constexpr std::uint64_t test_seed = 1234567;

	static_assert( std::uniform_random_bit_generator<mclo::xoshiro256plusplus>,
				   "xoshiro256plusplus is should model the uniform random bit generator concept" );
}

TEST_CASE( "xoshiro256plusplus expected output", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro( test_seed );
	CHECK( xoshiro() == 437095814655224680 );
	CHECK( xoshiro() == 8127161015984454572 );
	CHECK( xoshiro() == 18128670339019551454 );
	CHECK( xoshiro() == 254746599813523466 );
	CHECK( xoshiro() == 6010839568078443526 );
}

TEST_CASE( "xoshiro256plusplus discard skips to expected output", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro( test_seed );
	xoshiro.discard( 3 );
	CHECK( xoshiro() == 254746599813523466 );
	CHECK( xoshiro() == 6010839568078443526 );
}

TEST_CASE( "xoshiro256plusplus seed resets to expected output", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro( test_seed );
	xoshiro();
	xoshiro.seed( test_seed );
	CHECK( xoshiro() == 437095814655224680 );
	CHECK( xoshiro() == 8127161015984454572 );
}

TEST_CASE( "xoshiro256plusplus min and max return expected values", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro;
	CHECK( xoshiro.min() == 0 );
	CHECK( xoshiro.max() == std::numeric_limits<mclo::xoshiro256plusplus::result_type>::max() );
}

TEST_CASE( "xoshiro256plusplus comparison operators", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro1( test_seed );
	mclo::xoshiro256plusplus xoshiro2( test_seed );
	CHECK( xoshiro1 == xoshiro2 );
	CHECK_FALSE( xoshiro1 != xoshiro2 );
}

TEST_CASE( "xoshiro256plusplus comparison operators with different seeds", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro1( test_seed );
	mclo::xoshiro256plusplus xoshiro2( test_seed + 1 );
	CHECK_FALSE( xoshiro1 == xoshiro2 );
	CHECK( xoshiro1 != xoshiro2 );
}

TEST_CASE( "xoshiro256plusplus comparison operators with different states", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro1( test_seed );
	mclo::xoshiro256plusplus xoshiro2( test_seed );
	xoshiro1();
	CHECK_FALSE( xoshiro1 == xoshiro2 );
	CHECK( xoshiro1 != xoshiro2 );
}

TEST_CASE( "xoshiro256plusplus with distribution", "[random]" )
{
	mclo::xoshiro256plusplus xoshiro( test_seed );
	std::uniform_int_distribution<> distribution( 0, 100 );

	std::size_t count = 100;
	while ( count-- )
	{
		const int value = distribution( xoshiro );
		// The distribution's exact implementation is not specified, so we can only check that the value is within the
		// expected range.
		CHECK( value >= 0 );
		CHECK( value <= 100 );
	}
}
