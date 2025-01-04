#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/meta/type_list.hpp"
#include "mclo/random/default_random_generator.hpp"

#include <array>
#include <numeric>

using namespace Catch::Matchers;

namespace
{
	constexpr std::uint64_t test_seed = 1234567;

	using test_types = mclo::meta::
		type_list<std::int16_t, std::int32_t, std::int64_t, std::uint16_t, std::uint32_t, std::uint64_t, float, double>;

	template <auto min, auto max>
	auto in_range()
	{
		using type = std::common_type_t<decltype( min ), decltype( max )>;
		return Predicate<type>( []( const type i ) { return i >= min && i <= max; } );
	}
}

TEMPLATE_LIST_TEST_CASE( "random_generator uniform", "[random]", test_types )
{
	static constexpr TestType min( 0 );
	static constexpr TestType max( 100 );
	mclo::default_random_generator rng( test_seed );
	std::size_t count = 10;
	while ( count-- )
	{
		CHECK_THAT( rng.uniform( min, max ), ( in_range<min, max>() ) );
	}
}

TEST_CASE( "random_generator generate", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<int, 10> arr;
	rng.generate( std::uniform_int_distribution<int>( 0, 100 ), arr );
	CHECK_THAT( arr, AllMatch( in_range<0, 100>() ) );
}

TEST_CASE( "random_generator generate default", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<int, 10> arr;
	rng.generate( arr );
	CHECK_THAT( arr, AllMatch( in_range<0, INT_MAX>() ) );
}

TEST_CASE( "random_generator generate real", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<float, 10> arr;
	rng.generate( std::uniform_real_distribution<float>( 0, 100 ), arr );
	CHECK_THAT( arr, AllMatch( in_range<0.0, 100.0>() ) );
}

TEST_CASE( "random_generator generate default real", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<float, 10> arr;
	rng.generate( arr );
	CHECK_THAT( arr, AllMatch( in_range<0, 1.0f>() ) );
}

TEST_CASE( "random_generator pick_it", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<int, 10> arr;
	std::iota( arr.begin(), arr.end(), 0 );
	const auto it = rng.pick_it( arr );
	REQUIRE( it != arr.end() );
	CHECK_THAT( arr, Contains( *it ) );
}

TEST_CASE( "random_generator pick_it empty", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<int, 0> arr{};
	const auto it = rng.pick_it( arr );
	CHECK( it == arr.end() );
}

TEST_CASE( "random_generator pick", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	std::array<int, 10> arr;
	std::iota( arr.begin(), arr.end(), 0 );
	const int& val = rng.pick( arr );
	CHECK_THAT( arr, Contains( val ) );
}

TEST_CASE( "random_generator chance", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	CHECK_FALSE( rng.chance( 0, 0 ) );
	CHECK_FALSE( rng.chance( 0, 1 ) );
	CHECK_FALSE( rng.chance( 1, 0 ) );

	CHECK( rng.chance( 1, 1 ) );
	CHECK( rng.chance( 1, 2 ) );
	CHECK( rng.chance( 2, 1 ) );

	CHECK_FALSE( rng.chance( 1, 6 ) );
}

TEST_CASE( "random_generator chance_one_in", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	CHECK( rng.chance_one_in( 6 ) );
}

TEST_CASE( "random_generator coin_toss", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	CHECK( rng.coin_toss() );
}

TEST_CASE( "random_generator percent_chance", "[random]" )
{
	mclo::default_random_generator rng( test_seed );
	CHECK( rng.percent_chance( 1.0f ) );
	CHECK_FALSE( rng.percent_chance( 0.0f ) );
	CHECK_FALSE( rng.percent_chance( 0.5f ) );
}
