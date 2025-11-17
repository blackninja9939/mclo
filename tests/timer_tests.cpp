#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/timer.hpp"

#include <thread>

TEST_CASE( "timer elapsed returns delta", "[timer]" )
{
	mclo::timer<> timer;
	using duration = mclo::timer<>::duration;
	const duration sleep_time( 2 );
	std::this_thread::sleep_for( sleep_time );

	const duration delta = timer.elapsed();

	CHECK( delta >= sleep_time );
}

TEST_CASE( "timer tick returns detla and sets point", "[timer]" )
{
	mclo::timer<> timer;
	using duration = mclo::timer<>::duration;
	const duration sleep_time( 2 );
	std::this_thread::sleep_for( sleep_time );

	const duration delta = timer.tick();
	const duration elapsed = timer.elapsed();

	CHECK( delta >= sleep_time );
	CHECK( elapsed < delta );
}
