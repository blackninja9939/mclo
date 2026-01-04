#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/work_stealing_deque.hpp"

#include "mclo/random/default_random_generator.hpp"

#include <thread>

namespace
{
	using test_queue = mclo::work_stealing_deque<int>;

	struct thief
	{
		std::thread m_thread;
		std::vector<int> m_stolen_values;
		mclo::default_random_generator m_rng;
	};

	void test_with_thieves( const std::size_t num_thieves )
	{
		static constexpr std::size_t value_count = mclo::pow2( 16u );
		const std::size_t thief_loop_count = value_count / num_thieves;
		test_queue queue( 4 );

		// Start thief threads
		const auto thieves = std::make_unique<thief[]>( num_thieves );
		for ( std::size_t thief_index = 0; thief_index < num_thieves; ++thief_index )
		{
			thief& t = thieves[ thief_index ];
			t.m_stolen_values.reserve( value_count );
			t.m_rng.seed( thief_index );
			t.m_thread = std::thread( [ &queue, &t, thief_loop_count ]() {
				for ( std::size_t index = 0; index < thief_loop_count; ++index )
				{
					if ( t.m_rng.coin_toss() )
					{
						if ( auto stolen = queue.steal() )
						{
							t.m_stolen_values.push_back( *stolen );
						}
					}
				}
			} );
		}

		// Push values
		std::vector<int> pushed_values;
		pushed_values.reserve( value_count );
		for ( int index = 0; index < value_count; ++index )
		{
			queue.push( index );
			pushed_values.push_back( index );
		}

		// Pop from own queue until queue is empty
		std::vector<int> all_taken_values;
		all_taken_values.reserve( value_count );

		while ( auto popped = queue.pop() )
		{
			all_taken_values.push_back( *popped );
		}

		// Join thief threads and collect their stolen values
		for ( int i = 0; i < num_thieves; ++i )
		{
			thief& t = thieves[ i ];
			t.m_thread.join();
			all_taken_values.insert( all_taken_values.end(), t.m_stolen_values.begin(), t.m_stolen_values.end() );
		}

		std::sort( all_taken_values.begin(), all_taken_values.end() );
		CHECK( all_taken_values == pushed_values );
	}
}

TEST_CASE( "empty work_stealing_deque, is empty", "[work_stealing_deque]" )
{
	test_queue queue( 1 );
	CHECK( queue.size() == 0u );
	CHECK( queue.empty() );
}

TEST_CASE( "empty work_stealing_deque, capacity, is correct", "[work_stealing_deque]" )
{
	SECTION( "Capacity 1" )
	{
		test_queue queue( 1 );
		CHECK( queue.capacity() == 1 );
	}
	SECTION( "Capacity 5" )
	{
		test_queue queue( 15 );
		CHECK( queue.capacity() >= 15 );
	}
}

TEST_CASE( "empty work_stealing_deque, push value, increases size", "[work_stealing_deque]" )
{
	test_queue queue( 1 );

	queue.push( 42 );

	CHECK( queue.capacity() == 1u );
	CHECK( queue.size() == 1u );
	CHECK_FALSE( queue.empty() );
}

TEST_CASE( "full work_stealing_deque, push more, succeeds and grows capacity", "[work_stealing_deque]" )
{
	test_queue queue( 1 );

	for ( int i = 0; i < 5; ++i )
	{
		queue.push( i );
	}

	CHECK( queue.capacity() >= 5u );
	CHECK( queue.size() == 5u );
	CHECK_FALSE( queue.empty() );
}

TEST_CASE( "empty work_stealing_deque, pop, is nullopt", "[work_stealing_deque]" )
{
	test_queue queue( 1 );

	const auto result = queue.pop();

	CHECK_FALSE( result );
	CHECK( queue.size() == 0u );
	CHECK( queue.empty() );
}

TEST_CASE( "empty work_stealing_deque, steal, is nullopt", "[work_stealing_deque]" )
{
	test_queue queue( 1 );

	const auto result = queue.steal();

	CHECK_FALSE( result );
	CHECK( queue.size() == 0u );
	CHECK( queue.empty() );
}

TEST_CASE( "work_stealing_deque with single value, pop, gets pushed value", "[work_stealing_deque]" )
{
	test_queue queue( 1 );
	queue.push( 42 );

	const auto result = queue.pop();

	REQUIRE( result );
	CHECK( *result == 42 );
}

TEST_CASE( "work_stealing_deque with single value, steal, gets pushed value", "[work_stealing_deque]" )
{
	test_queue queue( 1 );
	queue.push( 42 );

	const auto result = queue.steal();

	REQUIRE( result );
	CHECK( *result == 42 );
}

TEST_CASE( "work_stealing_deque with multiple values, pop values, takes from top is order", "[work_stealing_deque]" )
{
	test_queue queue( 1 );

	for ( int i = 0; i < 5; ++i )
	{
		queue.push( i );
	}

	for ( int i = 5; i != 0; --i )
	{
		const auto result = queue.pop();
		REQUIRE( result );
		CHECK( *result == i - 1 );
	}
}

TEST_CASE( "work_stealing_deque with multiple values, steal values, takes from top in order", "[work_stealing_deque]" )
{
	test_queue queue( 1 );

	for ( int i = 0; i < 5; ++i )
	{
		queue.push( i );
	}

	for ( int i = 0; i < 5; ++i )
	{
		const auto result = queue.steal();
		REQUIRE( result );
		CHECK( *result == i );
	}
}

TEST_CASE( "work_stealing_deque with multiple threads, operations on threads, is expected values",
		   "[work_stealing_deque]" )
{
	test_queue queue( 1 );
	queue.push( 42 );

	const auto result = queue.steal();

	REQUIRE( result );
	CHECK( *result == 42 );
}

TEST_CASE( "work_stealing_queue test with 1 thief", "[work_stealing_deque]" )
{
	test_with_thieves( 1 );
}

TEST_CASE( "work_stealing_queue test with 2 thieves", "[work_stealing_deque]" )
{
	test_with_thieves( 2 );
}

TEST_CASE( "work_stealing_queue test with 4 thieves", "[work_stealing_deque]" )
{
	test_with_thieves( 4 );
}

TEST_CASE( "work_stealing_queue test with 8 thieves", "[work_stealing_deque]" )
{
	test_with_thieves( 8 );
}
