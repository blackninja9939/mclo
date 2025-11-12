#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/timer.hpp"

TEST_CASE( "timer elapsed returns delta", "[timer]" )
{
	mclo::timer<> timer;
	using duration = mclo::timer<>::duration;

	const duration delta = timer.elapsed();

	CHECK( delta > duration() );
}

TEST_CASE( "timer tick returns detla and sets point", "[timer]" )
{
	mclo::timer<> timer;
	using duration = mclo::timer<>::duration;

	const duration delta = timer.tick();

	CHECK( delta > duration() );
}
