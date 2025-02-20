#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/thread_local_key.hpp"

#include <thread>

TEST_CASE( "thread_local_key default gets nullptr", "[thread_local_key]" )
{
	mclo::thread_local_key key;
	CHECK( key.get() == nullptr );
}

TEST_CASE( "thread_local_key set changes get result", "[thread_local_key]" )
{
	mclo::thread_local_key key;
	int i = 4;
	key.set( &i );

	CHECK( key.get() == &i );
}

TEST_CASE( "thread_local_key set on one thread does not change other", "[thread_local_key]" )
{
	mclo::thread_local_key key;
	int i = 4;
	key.set( &i );

	std::jthread thread( [ &key ] { CHECK( key.get() == nullptr ); } );
}
