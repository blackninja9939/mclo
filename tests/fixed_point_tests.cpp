#include <catch2/catch_test_macros.hpp>

#include "mclo/fixed_point.hpp"
#include "mclo/meta/type_list.hpp"
#include "mclo/meta/join.hpp"

namespace
{
	using namespace mclo::fixed_point_literals;

	using unsigned_fixed = mclo::meta::type_list<mclo::fixedu32, mclo::fixedu64>;
	using signed_fixed = mclo::meta::type_list<mclo::fixed32, mclo::fixed64>;

	using fixed_types = mclo::meta::join<unsigned_fixed, unsigned_fixed>;
}

TEST_CASE( "decimal_fixed_point default", "[fixed_point]" )
{
	mclo::fixed32 value;
	CHECK( value.truncated() == 0 );
}

TEST_CASE( "decimal_fixed_point integer initializer", "[fixed_point]" )
{
	mclo::fixed32 value(16);
	CHECK( value.truncated() == 16 );
}

TEST_CASE( "decimal_fixed_point float initializer", "[fixed_point]" )
{
	mclo::fixed32 value( 16.5 );
	CHECK( value.truncated() == 16 );
}

TEST_CASE( "decimal_fixed_point numeric operators", "[fixed_point]" )
{
	mclo::fixed32 value( 16.5 );
	
	++value;
	CHECK( value == 17.5_fixed32 );
}
