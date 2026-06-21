#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/atomic128.hpp"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

namespace
{
	struct pair
	{
		std::uint64_t a;
		std::uint64_t b;

		bool operator==( const pair& ) const = default;
	};
}

TEST_CASE( "atomic128 default constructs to zero", "[atomic128]" )
{
	const mclo::atomic128<pair> value;
	CHECK( value.load() == pair{ 0, 0 } );
}

TEST_CASE( "atomic128 is lock free", "[atomic128]" )
{
	const mclo::atomic128<pair> value;
	CHECK( value.is_lock_free() );
	CHECK( mclo::atomic128<pair>::is_always_lock_free );
}

TEST_CASE( "atomic128 store then load", "[atomic128]" )
{
	mclo::atomic128<pair> value;
	value.store( pair{ 3, 4 } );
	CHECK( value.load() == pair{ 3, 4 } );
}

TEST_CASE( "atomic128 exchange returns previous value", "[atomic128]" )
{
	mclo::atomic128<pair> value( pair{ 1, 2 } );
	const pair old = value.exchange( pair{ 5, 6 } );
	CHECK( old == pair{ 1, 2 } );
	CHECK( value.load() == pair{ 5, 6 } );
}

TEST_CASE( "atomic128 compare_exchange success", "[atomic128]" )
{
	mclo::atomic128<pair> value( pair{ 7, 8 } );
	pair expected{ 7, 8 };
	CHECK( value.compare_exchange_strong( expected, pair{ 9, 10 } ) );
	CHECK( value.load() == pair{ 9, 10 } );
}

TEST_CASE( "atomic128 compare_exchange failure updates expected", "[atomic128]" )
{
	mclo::atomic128<pair> value( pair{ 7, 8 } );
	pair expected{ 0, 0 };
	CHECK_FALSE( value.compare_exchange_strong( expected, pair{ 9, 10 } ) );
	CHECK( expected == pair{ 7, 8 } );
	CHECK( value.load() == pair{ 7, 8 } );
}

TEST_CASE( "atomic128 concurrent increments do not tear", "[atomic128]" )
{
	mclo::atomic128<pair> value( pair{ 0, 0 } );

	constexpr int num_threads = 4;
	constexpr int per_thread = 100000;

	std::vector<std::thread> threads;
	for ( int t = 0; t < num_threads; ++t )
	{
		threads.emplace_back( [ &value ] {
			for ( int i = 0; i < per_thread; ++i )
			{
				pair expected = value.load( std::memory_order_relaxed );
				pair desired;
				do
				{
					desired = pair{ expected.a + 1, expected.b + 1 };
				}
				while ( !value.compare_exchange_weak(
					expected, desired, std::memory_order_relaxed, std::memory_order_relaxed ) );
			}
		} );
	}
	for ( auto& thread : threads )
	{
		thread.join();
	}

	const pair result = value.load();
	// The two halves are always incremented together so they must remain equal, proving no torn updates occurred.
	CHECK( result.a == static_cast<std::uint64_t>( num_threads ) * per_thread );
	CHECK( result.a == result.b );
}
