#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/container/intrusive_forward_list.hpp"

#include <ranges>
#include <vector>

using namespace Catch::Matchers;

struct test_type : mclo::intrusive_forward_list_hook<>
{
	test_type( int i ) noexcept
		: i( i )
	{
	}

	int i = 0;

	bool operator==( const test_type& other ) const noexcept
	{
		return i == other.i;
	}
};

TEST_CASE( "Default intrusive forward list is empty", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;
	CHECK( list.empty() );
	CHECK( std::ranges::distance( list ) == 0 );
}

TEST_CASE( "Empty intrusive forward list, pop front, returns nullptr", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;

	const test_type* const ptr = list.pop_front();

	CHECK_FALSE( ptr );
}

TEST_CASE( "Intrusive forward list push front contains element", "[intrusive][intrusive_forward_list]" )
{
	test_type object{ 3 };
	mclo::intrusive_forward_list<test_type> list;

	list.push_front( object );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 1 );
	CHECK_THAT( list, RangeEquals( std::array{ object } ) );
}

TEST_CASE( "Intrusive forward list, push front multiple, contains elements in reverse insertion order",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list;

	for ( auto& obj : objects )
	{
		list.push_front( obj );
	}

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( std::views::reverse( objects ) ) );
}

TEST_CASE( "Non-empty intrusive forward list, pop front, first element removed rest in reverse insertion order",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list;
	for ( auto& obj : objects )
	{
		list.push_front( obj );
	}

	const test_type* const ptr = list.pop_front();

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 4 );
	CHECK_THAT( list, RangeEquals( objects | std::views::reverse | std::views::drop( 1 ) ) );
	REQUIRE( ptr );
	CHECK( ptr->i == 4 );
}

TEST_CASE( "Non-empty intrusive forward list, clear, is empty", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list;
	for ( auto& obj : objects )
	{
		list.push_front( obj );
	}

	list.clear();

	CHECK( list.empty() );
	CHECK( std::ranges::distance( list ) == 0 );
}

TEST_CASE( "Intrusive forward list, insert after before begin, contains element",
		   "[intrusive][intrusive_forward_list]" )
{
	test_type object{ 3 };
	mclo::intrusive_forward_list<test_type> list;

	list.insert_after( list.before_begin(), object );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 1 );
	CHECK_THAT( list, RangeEquals( std::array{ object } ) );
}

TEST_CASE( "Non-empty intrusive forward list, insert after begin, contains element after begin",
		   "[intrusive][intrusive_forward_list]" )
{
	test_type object{ 3 };
	test_type object2{ 4 };
	mclo::intrusive_forward_list<test_type> list;
	list.insert_after( list.before_begin(), object );

	list.insert_after( list.begin(), object2 );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 2 );
	CHECK_THAT( list, RangeEquals( std::array{ object, object2 } ) );
}
