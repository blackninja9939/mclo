#include <catch2/catch_test_macros.hpp>

#include "mclo/random/weighted_random.hpp"
#include "mclo/random/xoshiro256plusplus.hpp"

#include <array>
#include <vector>

namespace
{
	constexpr std::uint64_t test_seed = 1234567;

	struct item
	{
		int weight;
	};
}

TEST_CASE( "weighted_index single element", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array weights{ 5 };
	CHECK( mclo::weighted_index( engine, weights ) == 0 );
}

TEST_CASE( "weighted_index identity raw weights", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array weights{ 0, 5, 0, 0 };
	for ( int i = 0; i < 100; ++i )
	{
		CHECK( mclo::weighted_index( engine, weights ) == 1 );
	}
}

TEST_CASE( "weighted_index precomputed total", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array weights{ 0, 5, 0, 0 };
	for ( int i = 0; i < 100; ++i )
	{
		CHECK( mclo::weighted_index( engine, weights, 5 ) == 1 );
	}
}

TEST_CASE( "weighted_index precomputed total with projection", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array<item, 3> items{
		{ { 0 }, { 7 }, { 0 } }
    };
	for ( int i = 0; i < 100; ++i )
	{
		CHECK( mclo::weighted_index( engine, items, 7, &item::weight ) == 1 );
	}
}

TEST_CASE( "weighted_index projection on demand", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array values{ 1, 2, 3, 4 };
	for ( int i = 0; i < 100; ++i )
	{
		const auto idx = mclo::weighted_index( engine, values, []( int v ) { return v == 3 ? 1 : 0; } );
		CHECK( idx == 2 );
	}
}

TEST_CASE( "weighted_index pointer to member", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array<item, 3> items{
		{ { 0 }, { 0 }, { 7 } }
    };
	for ( int i = 0; i < 100; ++i )
	{
		CHECK( mclo::weighted_index( engine, items, &item::weight ) == 2 );
	}
}

TEST_CASE( "weighted_index parallel weights container", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array values{ 'a', 'b', 'c' };
	const std::array weights{ 0, 0, 9 };
	for ( int i = 0; i < 100; ++i )
	{
		const auto idx = mclo::weighted_index( engine, weights );
		CHECK( idx == 2 );
		CHECK( values[ idx ] == 'c' );
	}
}

TEST_CASE( "weighted_index distribution bias", "[random][weighted_random]" )
{
	mclo::xoshiro256plusplus engine( test_seed );
	const std::array weights{ 1, 9 };
	int counts[ 2 ] = { 0, 0 };
	for ( int i = 0; i < 10000; ++i )
	{
		++counts[ mclo::weighted_index( engine, weights ) ];
	}
	CHECK( counts[ 1 ] > counts[ 0 ] );
}
