#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/instanced_thread_local.hpp"

#include <thread>
#include <unordered_set>
#include <vector>

TEST_CASE( "instanced_thread_local get are unique across threads", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local<int> object;
	int& i = object.get();

	std::jthread thread( [ & ] { CHECK( &object.get() != &i ); } );
}

TEST_CASE( "instanced_thread_local dereference same as get", "[mutex]" )
{
	mclo::instanced_thread_local<int> object;
	CHECK( &object.get() == &*object );
}

TEST_CASE( "instanced_thread_local arrow operator same as get", "[mutex]" )
{
	mclo::instanced_thread_local<int> object;
	CHECK( &object.get() == object.operator->() );
}

TEST_CASE( "instanced_thread_local iterate over contains all threads", "[mutex]" )
{
	mclo::instanced_thread_local<int> object;
	std::unordered_set<int> thread_values;
	thread_values.insert( object.get() = 0 );
	std::vector<std::jthread> threads;

	for ( int i = 1; i < 10; ++i )
	{
		thread_values.insert( i );
		threads.emplace_back( [ &object, i ] { object.get() = i; } );
	}

	threads.clear();

	for ( auto& value : object )
	{
		const std::size_t erased = thread_values.erase( value );
		CHECK( erased == 1 );
	}
	CHECK( thread_values.empty() );
}

TEST_CASE( "instanced_thread_local_value get is zero initialized", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local_value<int> object;
	CHECK( object.get() == 0 );
}

TEST_CASE( "instanced_thread_local_value set changes get result", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local_value<int> object;
	object.set( 4 );
	CHECK( object.get() == 4 );
}

TEST_CASE( "instanced_thread_local_value get are unique across threads", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local_value<int> object;
	object.set( 4 );

	std::jthread thread( [ & ] { CHECK( object.get() != 4 ); } );
}
