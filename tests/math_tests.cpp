#include "consteval_check.hpp"

#include "assert_macros.hpp"

#include "mclo/numeric/log2.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/numeric/pow10.hpp"
#include "mclo/numeric/pow2.hpp"

TEST_CASE( "ceil_divide", "[math]" )
{
	CHECK_ASSERTS( mclo::ceil_divide( 6, 0 ), "Divizion by zero" );
	CONSTEVAL_CHECK( mclo::ceil_divide( 6, 4 ) == 2 );
	CONSTEVAL_CHECK( mclo::ceil_divide( 6, -4 ) == -1 );
	CONSTEVAL_CHECK( mclo::ceil_divide( -6, 4 ) == -1 );
	CONSTEVAL_CHECK( mclo::ceil_divide( -6, -4 ) == 2 );
	CONSTEVAL_CHECK( mclo::ceil_divide( 6u, 4u ) == 2 );
}

TEST_CASE( "round_down_to_multiple_of", "[math]" )
{
	CONSTEVAL_CHECK( mclo::round_down_to_multiple_of( 6, 4 ) == 4 );
	CONSTEVAL_CHECK( mclo::round_down_to_multiple_of( 6, -4 ) == 4 );
	CONSTEVAL_CHECK( mclo::round_down_to_multiple_of( -6, 4 ) == -4 );
	CONSTEVAL_CHECK( mclo::round_down_to_multiple_of( -6, -4 ) == -4 );
}

TEST_CASE( "log2_floor", "[math]" )
{
	CHECK_ASSERTS( mclo::log2_floor( 0u ), "log2(0) is undefined" );
	CONSTEVAL_CHECK( mclo::log2_floor( 1u ) == 0 );
	CONSTEVAL_CHECK( mclo::log2_floor( 2u ) == 1 );
	CONSTEVAL_CHECK( mclo::log2_floor( 3u ) == 1 );
	CONSTEVAL_CHECK( mclo::log2_floor( 4u ) == 2 );
	CONSTEVAL_CHECK( mclo::log2_floor( 5u ) == 2 );
	CONSTEVAL_CHECK( mclo::log2_floor( 8u ) == 3 );
}

TEST_CASE( "log2_ceil", "[math]" )
{
	CHECK_ASSERTS( mclo::log2_ceil( 0u ), "log2(0) is undefined" );
	CONSTEVAL_CHECK( mclo::log2_ceil( 1u ) == 0 );
	CONSTEVAL_CHECK( mclo::log2_ceil( 2u ) == 1 );
	CONSTEVAL_CHECK( mclo::log2_ceil( 3u ) == 2 );
	CONSTEVAL_CHECK( mclo::log2_ceil( 4u ) == 2 );
	CONSTEVAL_CHECK( mclo::log2_ceil( 5u ) == 3 );
	CONSTEVAL_CHECK( mclo::log2_ceil( 8u ) == 3 );
}

TEST_CASE( "pow2", "[math]" )
{
	CONSTEVAL_CHECK( mclo::pow2( 0u ) == 1 );
	CONSTEVAL_CHECK( mclo::pow2( 1u ) == 2 );
	CONSTEVAL_CHECK( mclo::pow2( 2u ) == 4 );
	CONSTEVAL_CHECK( mclo::pow2( 8u ) == 256 );
}

TEST_CASE( "is_pow2", "[math]" )
{
	CONSTEVAL_CHECK_FALSE( mclo::is_pow2( 0u ) );
	CONSTEVAL_CHECK( mclo::is_pow2( 1u ) );
	CONSTEVAL_CHECK( mclo::is_pow2( 2u ) );
	CONSTEVAL_CHECK_FALSE( mclo::is_pow2( 3u ) );
	CONSTEVAL_CHECK( mclo::is_pow2( 4u ) );
	CONSTEVAL_CHECK( mclo::is_pow2( 256u ) );
	CONSTEVAL_CHECK_FALSE( mclo::is_pow2( 99u ) );
}

TEST_CASE( "pow10", "[math]" )
{
	CONSTEVAL_CHECK( mclo::pow10( 0u ) == 1ull );
	CONSTEVAL_CHECK( mclo::pow10( 1u ) == 10ull );
	CONSTEVAL_CHECK( mclo::pow10( 2u ) == 100ull );
	CONSTEVAL_CHECK( mclo::pow10( 8u ) == 100000000ull );
	CONSTEVAL_CHECK( mclo::pow10( 11u ) == 100000000000ull );
	CONSTEVAL_CHECK( mclo::pow10( 19u ) == 10000000000000000000ull );
}
