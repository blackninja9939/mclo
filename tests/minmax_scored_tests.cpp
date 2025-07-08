#include <catch2/catch_test_macros.hpp>

#include "mclo/algorithm/minmax_scored.hpp"

namespace
{
	struct counted_identity
	{
		template <typename T>
		constexpr T&& operator()( T&& value ) const
		{
			++calls;
			return std::forward<T>( value );
		}

		std::size_t& calls;
	};
}

TEST_CASE( "min_scored on empty finds last", "[minmax_scored]" )
{
	const std::vector<int> values;

	const auto min_it = mclo::min_scored( values.begin(), values.end(), std::identity{} );

	REQUIRE( min_it == values.end() );
}

TEST_CASE( "max_scored on empty finds last", "[minmax_scored]" )
{
	const std::vector<int> values;

	const auto max_it = mclo::max_scored( values.begin(), values.end(), std::identity{} );

	REQUIRE( max_it == values.end() );
}

TEST_CASE( "minmax_scored on empty finds last", "[minmax_scored]" )
{
	const std::vector<int> values;

	const auto [ min_it, max_it ] = mclo::minmax_scored( values.begin(), values.end(), std::identity{} );

	REQUIRE( min_it == values.end() );
	REQUIRE( max_it == values.end() );
}

TEST_CASE( "min_scored finds lowest scoring", "[minmax_scored]" )
{
	const std::vector<int> values{ 5, 3, 8, 1, 4 };

	const auto min_it = mclo::min_scored( values.begin(), values.end(), std::identity{} );

	REQUIRE( min_it != values.end() );
	CHECK( *min_it == 1 );
}

TEST_CASE( "max_scored finds highest scoring", "[minmax_scored]" )
{
	const std::vector<int> values{ 5, 3, 8, 1, 4 };

	const auto max_it = mclo::max_scored( values.begin(), values.end(), std::identity{} );

	REQUIRE( max_it != values.end() );
	CHECK( *max_it == 8 );
}

TEST_CASE( "minmax_scored finds min and max", "[minmax_scored]" )
{
	const std::vector<int> values{ 5, 3, 8, 1, 4 };

	const auto [ min_it, max_it ] = mclo::minmax_scored( values.begin(), values.end(), std::identity{} );

	REQUIRE( min_it != values.end() );
	REQUIRE( max_it != values.end() );
	CHECK( *min_it == 1 );
	CHECK( *max_it == 8 );
}

TEST_CASE( "min_scored with custom scorer", "[minmax_scored]" )
{
	using pair_t = std::pair<int, int>;
	const std::vector<pair_t> values{
		{1, 5},
        {2, 3},
        {3, 8},
        {4, 1},
        {5, 4}
    };

	const auto min_it = mclo::min_scored( values.begin(), values.end(), &pair_t::second );

	REQUIRE( min_it != values.end() );
	CHECK( min_it->first == 4 ); // The pair with the lowest second value
}

TEST_CASE( "max_scored with custom scorer", "[minmax_scored]" )
{
	using pair_t = std::pair<int, int>;
	const std::vector<pair_t> values{
		{1, 5},
        {2, 3},
        {3, 8},
        {4, 1},
        {5, 4}
    };

	const auto max_it = mclo::max_scored( values.begin(), values.end(), &pair_t::second );

	REQUIRE( max_it != values.end() );
	CHECK( max_it->first == 3 ); // The pair with the highest second value
}

TEST_CASE( "minmax_scored with custom scorer", "[minmax_scored]" )
{
	using pair_t = std::pair<int, int>;
	const std::vector<pair_t> values{
		{1, 5},
        {2, 3},
        {3, 8},
        {4, 1},
        {5, 4}
    };

	const auto [ min_it, max_it ] = mclo::minmax_scored( values.begin(), values.end(), &pair_t::second );

	REQUIRE( min_it != values.end() );
	REQUIRE( max_it != values.end() );
	CHECK( min_it->first == 4 ); // The pair with the lowest second value
	CHECK( max_it->first == 3 ); // The pair with the highest second value
}

TEST_CASE( "min_scored calls predicate once per element", "[minmax_scored]" )
{
	const std::vector<int> values{ 5, 3, 8, 1, 4 };
	std::size_t calls = 0;

	const auto min_it = mclo::min_scored( values.begin(), values.end(), counted_identity{ calls } );

	REQUIRE( min_it != values.end() );
	CHECK( *min_it == 1 );
	CHECK( calls == values.size() );
}

TEST_CASE( "max_scored calls predicate once per element", "[minmax_scored]" )
{
	const std::vector<int> values{ 5, 3, 8, 1, 4 };
	std::size_t calls = 0;

	const auto max_it = mclo::max_scored( values.begin(), values.end(), counted_identity{ calls } );

	REQUIRE( max_it != values.end() );
	CHECK( *max_it == 8 );
	CHECK( calls == values.size() );
}

TEST_CASE( "minmax_scored calls predicate once per element", "[minmax_scored]" )
{
	const std::vector<int> values{ 5, 3, 8, 1, 4 };
	std::size_t calls = 0;

	const auto [ min_it, max_it ] = mclo::minmax_scored( values.begin(), values.end(), counted_identity{ calls } );

	REQUIRE( min_it != values.end() );
	REQUIRE( max_it != values.end() );
	CHECK( *min_it == 1 );
	CHECK( *max_it == 8 );
	CHECK( calls == values.size() );
}
