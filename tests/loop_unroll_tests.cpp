#include <catch2/catch_test_macros.hpp>

#include <catch2/matchers/catch_matchers_range_equals.hpp>

#include "mclo/utility/loop_unroll.hpp"

#include <vector>

using namespace Catch::Matchers;

TEST_CASE( "loop_unroll calls expected number of times from smallest to largest idnex", "[utility]" )
{
	std::vector<std::size_t> values;

	mclo::loop_unroll<5>( [ & ]( const auto i ) { values.push_back( i ); } );

	CHECK_THAT( values, RangeEquals( { 0, 1, 2, 3, 4 } ) );
}
