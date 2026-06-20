#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_list.hpp"
#include "mclo/threading/condition_variable.hpp"
#include "mclo/threading/mutex.hpp"
#include "mclo/threading/spin_mutex.hpp"
#include "mclo/threading/synchronized.hpp"

#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
	using mutex_types = mclo::meta::type_list<mclo::mutex, mclo::spin_mutex, std::shared_mutex>;
}

TEMPLATE_LIST_TEST_CASE( "default constructed synchronized, copy, has value typed default",
						 "[synchronized]",
						 mutex_types )
{
	mclo::synchronized<std::string, TestType> sync;

	const std::string value = sync.copy();

	CHECK( value.empty() );
}

TEMPLATE_LIST_TEST_CASE( "synchronized constructed from value, copy, returns that value",
						 "[synchronized]",
						 mutex_types )
{
	mclo::synchronized<int, TestType> sync( 42 );

	const int value = sync.copy();

	CHECK( value == 42 );
}

TEMPLATE_LIST_TEST_CASE( "synchronized constructed in place, copy, returns constructed value",
						 "[synchronized]",
						 mutex_types )
{
	mclo::synchronized<std::string, TestType> sync( std::in_place, 3, 'a' );

	const std::string value = sync.copy();

	CHECK( value == "aaa" );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, with_lock mutating value, mutation is observed", "[synchronized]", mutex_types )
{
	mclo::synchronized<int, TestType> sync( 1 );

	sync.with_lock( []( int& value ) { value += 41; } );

	CHECK( sync.copy() == 42 );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, with_lock returning value, returns callable result",
						 "[synchronized]",
						 mutex_types )
{
	mclo::synchronized<int, TestType> sync( 20 );

	const int result = sync.with_lock( []( int& value ) { return value * 2; } );

	CHECK( result == 40 );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, with_shared_lock reading value, returns value", "[synchronized]", mutex_types )
{
	mclo::synchronized<int, TestType> sync( 7 );

	const int value = sync.with_shared_lock( []( const int& v ) { return v; } );

	CHECK( value == 7 );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, with_shared_lock given member function pointer, invokes member",
						 "[synchronized]",
						 mutex_types )
{
	mclo::synchronized<std::string, TestType> sync( std::in_place, 3, 'a' );

	const std::size_t size = sync.with_shared_lock( &std::string::size );

	CHECK( size == 3 );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, copy_assigned value, copy returns new value", "[synchronized]", mutex_types )
{
	mclo::synchronized<int, TestType> sync( 1 );

	const int new_value = 99;
	sync = new_value;

	CHECK( sync.copy() == 99 );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, move_assigned value, copy returns new value", "[synchronized]", mutex_types )
{
	mclo::synchronized<std::string, TestType> sync( "old" );

	sync = std::string( "new" );

	CHECK( sync.copy() == "new" );
}

TEMPLATE_LIST_TEST_CASE( "synchronized, copy_into destination, destination holds value", "[synchronized]", mutex_types )
{
	mclo::synchronized<int, TestType> sync( 33 );

	int dest = 0;
	sync.copy_into( dest );

	CHECK( dest == 33 );
}

TEMPLATE_LIST_TEST_CASE( "two synchronized objects, swap, values are exchanged", "[synchronized]", mutex_types )
{
	mclo::synchronized<int, TestType> a( 1 );
	mclo::synchronized<int, TestType> b( 2 );

	a.swap( b );

	CHECK( a.copy() == 2 );
	CHECK( b.copy() == 1 );
}

TEMPLATE_LIST_TEST_CASE( "two synchronized objects, free swap, values are exchanged", "[synchronized]", mutex_types )
{
	mclo::synchronized<int, TestType> a( 1 );
	mclo::synchronized<int, TestType> b( 2 );

	using std::swap;
	swap( a, b );

	CHECK( a.copy() == 2 );
	CHECK( b.copy() == 1 );
}

TEST_CASE( "synchronized, concurrent with_lock increments, all increments are applied", "[synchronized]" )
{
	mclo::synchronized<int> sync( 0 );
	constexpr int num_threads = 8;
	constexpr int increments_per_thread = 1000;

	std::vector<std::thread> threads;
	for ( int i = 0; i < num_threads; ++i )
	{
		threads.emplace_back( [ &sync ] {
			for ( int j = 0; j < increments_per_thread; ++j )
			{
				sync.with_lock( []( int& value ) { ++value; } );
			}
		} );
	}
	for ( std::thread& thread : threads )
	{
		thread.join();
	}

	CHECK( sync.copy() == num_threads * increments_per_thread );
}

TEST_CASE( "synchronized with mutex and condition variable, with_lock waits until notified, receives produced item",
		   "[synchronized]" )
{
	mclo::synchronized<std::queue<int>, mclo::mutex> queue;
	mclo::condition_variable not_empty;

	std::thread producer( [ & ] {
		queue.with_lock( []( std::queue<int>& q ) { q.push( 42 ); } );
		not_empty.notify_one();
	} );

	const int item = queue.with_lock( [ & ]( std::queue<int>& q, std::unique_lock<mclo::mutex>& lock ) {
		not_empty.wait( lock, [ & ] { return !q.empty(); } );
		const int front = q.front();
		q.pop();
		return front;
	} );

	producer.join();

	CHECK( item == 42 );
}

static_assert( std::is_same_v<mclo::synchronized<int>::value_type, int> );
static_assert( std::is_same_v<mclo::synchronized<int>::mutex_type, std::shared_mutex> );
static_assert( std::is_same_v<mclo::synchronized<int, mclo::mutex>::mutex_type, mclo::mutex> );
