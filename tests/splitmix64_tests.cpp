#include <catch2/catch_test_macros.hpp>

#include "mclo/random/splitmix64.hpp"

#include <random>

namespace
{
	constexpr std::uint64_t test_seed = 1234567;

	static_assert( std::uniform_random_bit_generator<mclo::splitmix64>,
				   "splitmix64 is should model the uniform random bit generator concept" );
}

TEST_CASE( "splitmix64 expected output", "[random]" )
{
	mclo::splitmix64 splitmix( test_seed );
	CHECK( splitmix() == 6457827717110365317 );
	CHECK( splitmix() == 3203168211198807973 );
	CHECK( splitmix() == 9817491932198370423 );
	CHECK( splitmix() == 4593380528125082431 );
	CHECK( splitmix() == 16408922859458223821 );
}

TEST_CASE( "splitmix64 discard skips to expected output", "[random]" )
{
	mclo::splitmix64 splitmix( test_seed );
	splitmix.discard( 3 );
	CHECK( splitmix() == 4593380528125082431 );
	CHECK( splitmix() == 16408922859458223821 );
}

TEST_CASE( "splitmix64 seed resets to expected output", "[random]" )
{
	mclo::splitmix64 splitmix( test_seed );
	splitmix();
	splitmix.seed( test_seed );
	CHECK( splitmix() == 6457827717110365317 );
	CHECK( splitmix() == 3203168211198807973 );
}

TEST_CASE( "splitmix64 min and max return expected values", "[random]" )
{
	mclo::splitmix64 splitmix;
	CHECK( splitmix.min() == 0 );
	CHECK( splitmix.max() == std::numeric_limits<mclo::splitmix64::result_type>::max() );
}

TEST_CASE( "splitmix64 comparison operators", "[random]" )
{
	mclo::splitmix64 splitmix1( test_seed );
	mclo::splitmix64 splitmix2( test_seed );
	CHECK( splitmix1 == splitmix2 );
	CHECK_FALSE( splitmix1 != splitmix2 );
}

TEST_CASE( "splitmix64 comparison operators with different seeds", "[random]" )
{
	mclo::splitmix64 splitmix1( test_seed );
	mclo::splitmix64 splitmix2( test_seed + 1 );
	CHECK_FALSE( splitmix1 == splitmix2 );
	CHECK( splitmix1 != splitmix2 );
}

TEST_CASE( "splitmix64 comparison operators with different states", "[random]" )
{
	mclo::splitmix64 splitmix1( test_seed );
	mclo::splitmix64 splitmix2( test_seed );
	splitmix1();
	CHECK_FALSE( splitmix1 == splitmix2 );
	CHECK( splitmix1 != splitmix2 );
}

TEST_CASE( "splitmix64 with distribution", "[random]" )
{
	mclo::splitmix64 splitmix( test_seed );
	std::uniform_int_distribution<> distribution( 0, 100 );

	std::size_t count = 100;
	while ( count-- )
	{
		const int value = distribution( splitmix );
		// The distribution's exact implementation is not specified, so we can only check that the value is within the
		// expected range.
		CHECK( value >= 0 );
		CHECK( value <= 100 );
	}
}
