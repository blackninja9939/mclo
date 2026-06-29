#include <catch2/catch_test_macros.hpp>

#include "mclo/random/weighted_vector.hpp"
#include "mclo/random/xoshiro256plusplus.hpp"

#include <string>
#include <vector>

namespace
{
	constexpr std::uint64_t test_seed = 1234567;
}

TEST_CASE( "weighted_vector emplace and access", "[random][weighted_random]" )
{
	mclo::weighted_vector<std::string> vec;
	vec.emplace_back( 3, "a" );
	vec.push_back( 5, "b" );

	REQUIRE( vec.size() == 2 );
	CHECK_FALSE( vec.empty() );
	CHECK( vec[ 0 ] == "a" );
	CHECK( vec[ 1 ] == "b" );
	CHECK( vec.weight( 0 ) == 3 );
	CHECK( vec.weight( 1 ) == 5 );
	CHECK( vec.total_weight() == 8 );
}

TEST_CASE( "weighted_vector set_weight", "[random][weighted_random]" )
{
	mclo::weighted_vector<int> vec;
	vec.emplace_back( 1, 10 );
	vec.emplace_back( 2, 20 );
	REQUIRE( vec.total_weight() == 3 );

	vec.set_weight( 0, 4 );
	CHECK( vec.weight( 0 ) == 4 );
	CHECK( vec.total_weight() == 6 );
}

TEST_CASE( "weighted_vector sample respects weights", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	mclo::weighted_vector<char> vec;
	vec.emplace_back( 0, 'a' );
	vec.emplace_back( 0, 'b' );
	vec.emplace_back( 7, 'c' );

	for ( int i = 0; i < 100; ++i )
	{
		const auto index = vec.sample( engine );
		CHECK( index == 2 );
		CHECK( vec[ index ] == 'c' );
	}
}

TEST_CASE( "weighted_vector swap_remove", "[random][weighted_random]" )
{
	mclo::weighted_vector<int> vec;
	vec.emplace_back( 1, 10 );
	vec.emplace_back( 2, 20 );
	vec.emplace_back( 3, 30 );

	vec.swap_remove( 0 );
	REQUIRE( vec.size() == 2 );
	CHECK( vec[ 0 ] == 30 );
	CHECK( vec.weight( 0 ) == 3 );
	CHECK( vec[ 1 ] == 20 );
	CHECK( vec.weight( 1 ) == 2 );
	CHECK( vec.total_weight() == 5 );
}

TEST_CASE( "weighted_vector swap_remove last", "[random][weighted_random]" )
{
	mclo::weighted_vector<int> vec;
	vec.emplace_back( 1, 10 );
	vec.emplace_back( 2, 20 );

	vec.swap_remove( 1 );
	REQUIRE( vec.size() == 1 );
	CHECK( vec[ 0 ] == 10 );
	CHECK( vec.weight( 0 ) == 1 );
}

TEST_CASE( "weighted_vector pop_sample", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	mclo::weighted_vector<int> vec;
	vec.emplace_back( 0, 1 );
	vec.emplace_back( 5, 2 );
	vec.emplace_back( 0, 3 );

	const int value = vec.pop_sample( engine );
	CHECK( value == 2 );
	REQUIRE( vec.size() == 2 );
	CHECK( vec.total_weight() == 0 );
}
