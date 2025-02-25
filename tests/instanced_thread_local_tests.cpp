#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/threading/instanced_thread_local.hpp"

#include "mclo/allocator/linear_allocator.hpp"
#include "mclo/allocator/null_resource.hpp"

#include <thread>
#include <unordered_set>
#include <vector>

using namespace Catch::Matchers;

namespace
{
	struct throwing_type
	{
		throwing_type( std::nothrow_t, int i ) noexcept
			: i( i )
		{
		}

		throwing_type()
		{
			i = count.fetch_add( 1, std::memory_order_relaxed );
			if ( i >= 2 )
			{
				throw std::logic_error( "Fail" );
			}
		}

		auto operator<=>( const throwing_type& ) const noexcept = default;

		static inline std::atomic_int count{ 0 };
		int i = 0;
	};
}

TEST_CASE( "instanced_thread_local get are unique across threads", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local<int> object;
	int& i = object.get();

	std::jthread thread( [ & ] { CHECK( &object.get() != &i ); } );
}

TEST_CASE( "instanced_thread_local dereference same as get", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local<int> object;
	CHECK( &object.get() == &*object );
}

TEST_CASE( "instanced_thread_local arrow operator same as get", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local<int> object;
	CHECK( &object.get() == object.operator->() );
}

TEST_CASE( "instanced_thread_local iterate over contains all threads", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local<int> object;
	object.get() = 0;
	std::vector<std::jthread> threads;

	for ( int i = 1; i < 10; ++i )
	{
		threads.emplace_back( [ &object, i ] { object.get() = i; } );
	}

	threads.clear();
	constexpr std::array<int, 10> expected = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	CHECK_THAT( object, UnorderedRangeEquals( expected ) );
}

TEST_CASE( "instanced_thread_local custom allocator", "[instanced_thread_local]" )
{
	mclo::inline_linear_allocator_resource<16> resource;
	mclo::instanced_thread_local<int, mclo::linear_allocator<int>> object( resource );
	{
		std::jthread a( [ & ] { object.get() = 1; } );
		std::jthread b( [ & ] { object.get() = 2; } );
	}
	object.get() = 0;
	CHECK( object.get_allocator() == resource );
}

TEST_CASE( "instanced_thread_local custom allocator throws strong exception guarantee", "[instanced_thread_local]" )
{
	mclo::inline_linear_allocator_resource<32> resource( mclo::null_memory_resource::instance() );
	mclo::instanced_thread_local<int, mclo::linear_allocator<int>> object( resource );
	{
		std::jthread a( [ & ] { object.get() = 1; } );
	}
	{
		std::jthread b( [ & ] { object.get() = 2; } );
	}

	CHECK_THROWS_AS( object.get(), std::bad_alloc );

	CHECK_THAT( object, UnorderedRangeEquals( std::array{ 1, 2 } ) );
}

TEST_CASE( "instanced_thread_local custom type throws strong exception guarantee", "[instanced_thread_local]" )
{
	mclo::instanced_thread_local<throwing_type> object;
	{
		std::jthread a( [ & ] { ( void )object.get(); } );
		std::jthread b( [ & ] { ( void )object.get(); } );
	}

	CHECK_THROWS_AS( object.get(), std::logic_error );

	CHECK_THAT(
		object,
		UnorderedRangeEquals( std::array{ throwing_type( std::nothrow, 0 ), throwing_type( std::nothrow, 1 ) } ) );
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
