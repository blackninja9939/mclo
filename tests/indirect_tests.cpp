#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/indirect.hpp"

namespace
{
	struct destroy_tracker
	{
		bool& m_destroyed;

		~destroy_tracker()
		{
			m_destroyed = true;
		}
	};
}

TEST_CASE( "mclo::indirect default construction", "[indirect]" )
{
	const mclo::indirect<int> object;
	CHECK( *object == 0 ); // Default constructed int is 0
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::indirect value construction", "[indirect]" )
{
	const mclo::indirect<int> object( 42 );
	CHECK( *object == 42 );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::indirect deduction guide", "[indirect]" )
{
	mclo::indirect object{ 42 };

	CHECK( *object == 42 );
	static_assert( std::same_as<decltype( object ), mclo::indirect<int>> );
}

TEST_CASE( "mclo::indirect in place construction", "[indirect]" )
{
	const mclo::indirect<std::string> object( std::in_place, 5, 'a' );
	CHECK( *object == "aaaaa" );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::indirect in place construction with initializer list", "[indirect]" )
{
	const mclo::indirect<std::string> object( std::in_place, { 'a', 'b', 'c', 'd', 'e' } );
	CHECK( *object == "abcde" );
	CHECK_FALSE( object.valueless_after_move() );
}

TEST_CASE( "mclo::indirect copy construction", "[indirect]" )
{
	mclo::indirect<int> object1{ 50 };

	mclo::indirect<int> object2 = object1;
	*object1 = 75;

	CHECK( *object1 == 75 );
	CHECK( *object2 == 50 );
	CHECK_FALSE( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::indirect move construction", "[indirect]" )
{
	mclo::indirect<int> object1{ 100 };
	mclo::indirect<int> object2 = std::move( object1 );

	CHECK( *object2 == 100 );
	CHECK( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::indirect copy assignment", "[indirect]" )
{
	mclo::indirect<int> object1{ 25 };
	mclo::indirect<int> object2;

	object2 = object1;

	*object1 = 50;
	CHECK( *object1 == 50 );
	CHECK( *object2 == 25 );
	CHECK_FALSE( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::indirect move assignment", "[indirect]" )
{
	mclo::indirect<int> object1{ 75 };
	mclo::indirect<int> object2;

	object2 = std::move( object1 );

	CHECK( *object2 == 75 );
	CHECK( object1.valueless_after_move() );
	CHECK_FALSE( object2.valueless_after_move() );
}

TEST_CASE( "mclo::indirect swapping", "[indirect]" )
{
	mclo::indirect<int> object1{ 10 };
	mclo::indirect<int> object2{ 20 };

	SECTION( "member swap" )
	{
		object1.swap( object2 );
		CHECK( *object1 == 20 );
		CHECK( *object2 == 10 );
	}
	SECTION( "free function swap" )
	{
		swap( object1, object2 );
		CHECK( *object1 == 20 );
		CHECK( *object2 == 10 );
	}
}

TEST_CASE( "mclo::indirect swapping with valueless", "[indirect]" )
{
	mclo::indirect<int> object1{ 30 };
	mclo::indirect<int> object2 = std::move( object1 );

	CHECK( object1.valueless_after_move() );

	std::swap( object1, object2 );

	CHECK( object2.valueless_after_move() );
	CHECK( *object1 == 30 );
}

TEST_CASE( "mclo::indirect dereference operators", "[indirect]" )
{
	struct Data
	{
		int value;
		void set( int v )
		{
			value = v;
		}
	};
	mclo::indirect<Data> object{ 5 };
	CHECK( object->value == 5 );

	object->set( 15 );

	CHECK( object->value == 15 );
}

TEST_CASE( "mclo::indirect destruction", "[indirect]" )
{
	bool was_destroyed = false;

	{
		mclo::indirect<destroy_tracker> object{ was_destroyed };
	}

	CHECK( was_destroyed );
}

TEST_CASE( "mclo::indirect get_allocator", "[indirect]" )
{
	mclo::indirect<int> object{ 42 };

	CHECK( object.get_allocator() == std::allocator<int>{} );
}

TEST_CASE( "mclo::indirect equality and comparison", "[indirect]" )
{
	mclo::indirect<int> object1{ 50 };
	mclo::indirect<int> object2{ 50 };
	mclo::indirect<int> object3{ 75 };
	mclo::indirect<int> object4 = std::move( object1 );

	CHECK( ( object2 == object3 ) == false );
	CHECK( object2 == 50 );
	CHECK( object3 != 50 );

	CHECK( object2 <=> object3 == std::strong_ordering::less );
	CHECK( object3 <=> object2 == std::strong_ordering::greater );
	CHECK( object2 <=> 50 == std::strong_ordering::equal );

	CHECK( object1.valueless_after_move() );
	CHECK( object4 == object2 );
	CHECK( object1 != object2 );
	CHECK( object1 != object3 );
	CHECK( object1 != object4 );
}
