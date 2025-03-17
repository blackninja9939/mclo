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
	auto operator<=>( const test_type& other ) const noexcept
	{
		return i <=> other.i;
	}
};

template <>
struct Catch::StringMaker<test_type> : Catch::StringMaker<int>
{
	static std::string convert( const test_type& value )
	{
		return StringMaker<int>::convert( value.i );
	}
};

TEST_CASE( "Default intrusive forward list is empty", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;
	CHECK( list.empty() );
	CHECK( std::ranges::distance( list ) == 0 );
}

TEST_CASE( "Intrusive forward list initialized with iterator pair containts elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };

	const mclo::intrusive_forward_list<test_type> list( objects.begin(), objects.end() );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "Intrusive forward list initialized with range containts elements", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };

	const mclo::intrusive_forward_list<test_type> list( objects );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects ) );
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

TEST_CASE( "Empty intrusive forward list, insert a range after before begin, contains elements in order",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list;

	list.insert_after( list.before_begin(), objects.begin(), objects.end() );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "Non-empty intrusive forward list, insert after range, inserted after pos",
		   "[intrusive][intrusive_forward_list]" )
{
	test_type object{ 10 };
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list;
	list.push_front( object );

	list.insert_after( list.begin(), objects.begin(), objects.end() );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 6 );
	std::vector check{ object };
	check.insert( check.end(), objects.begin(), objects.end() );
	CHECK_THAT( list, RangeEquals( check ) );
}

TEST_CASE( "Non-empty intrusive forward list, reverse, contains reversed elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list( objects );

	list.reverse();

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects | std::views::reverse ) );
}

TEST_CASE( "mclo::intrusive_forward_list::splice_after - Move entire list", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> fl1( objects.begin(), objects.begin() + 3 );
	mclo::intrusive_forward_list<test_type> fl2( objects.begin() + 3, objects.end() );

	fl1.splice_after( fl1.before_begin(), fl2 );

	CHECK( fl2.empty() );
	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 4, 5, 6, 1, 2, 3 } ) );
}

TEST_CASE( "mclo::intrusive_forward_list::splice_after - Move single element", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> fl1( objects.begin(), objects.begin() + 3 );
	mclo::intrusive_forward_list<test_type> fl2( objects.begin() + 3, objects.end() );

	auto it = fl2.before_begin();
	fl1.splice_after( fl1.before_begin(), fl2, it );

	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 4, 1, 2, 3 } ) );
	CHECK_THAT( fl2, RangeEquals( std::vector<test_type>{ 5, 6 } ) );
}

TEST_CASE( "mclo::intrusive_forward_list::splice_after - Move range of elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5, 6, 7, 8 };
	mclo::intrusive_forward_list<test_type> fl1( objects.begin(), objects.begin() + 3 );
	mclo::intrusive_forward_list<test_type> fl2( objects.begin() + 3, objects.end() );

	auto it_start = fl2.begin();
	auto it_end = std::next( it_start, 3 );
	fl1.splice_after( fl1.before_begin(), fl2, it_start, it_end );

	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 5, 6, 1, 2, 3 } ) );
	CHECK_THAT( fl2, RangeEquals( std::vector<test_type>{ 4, 7, 8 } ) );
}

TEST_CASE( "mclo::intrusive_forward_list::splice_after - Move elements to empty list",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> fl1;
	mclo::intrusive_forward_list<test_type> fl2( objects );

	fl1.splice_after( fl1.before_begin(), fl2 );

	CHECK( fl2.empty() );
	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 4, 5, 6 } ) );
}

TEST_CASE( "mclo::intrusive_forward_list::splice_after - Self-splice (no-op)", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };
	mclo::intrusive_forward_list<test_type> fl;
	fl.assign( objects.begin(), objects.end() );

	auto it = fl.before_begin();
	fl.splice_after( it, fl, it );
	CHECK_THAT( fl, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5 } ) );
}

TEST_CASE( "Intrusive forward list no duplicates, unique, unchanged", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 2, 3, 4, 5 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 0 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "Intrusive forward list all elements identical, unique, removed all bubt one",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 1, 1, 1, 1 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 4 );
	CHECK_THAT( list, RangeEquals( std::vector<test_type>{ 1 } ) );
}

TEST_CASE( "Intrusive forward list some consecutive duplicates, unique, removed duplicates",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 1, 2, 2, 3, 3, 3, 4, 5, 5 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 5 );
	CHECK_THAT( list, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5 } ) );
}

TEST_CASE( "Intrusive forward list duplicates but non-consecutive, unique, unchanged",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 2, 1, 3, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 0 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "Intrusive forward list empty list, unique, unchanged", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;

	const std::size_t removed = list.unique();

	CHECK( removed == 0ull );
	CHECK( list.empty() );
}

//TEST_CASE( "mclo::intrusive_forward_list::sort - Already sorted list", "[intrusive][intrusive_forward_list]" )
//{
//	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };
//	mclo::intrusive_forward_list<test_type> fl( objects );
//
//	fl.sort();
//
//	CHECK_THAT( fl, RangeEquals( objects ) );
//}
//
//TEST_CASE( "mclo::intrusive_forward_list::sort - Reverse sorted list", "[intrusive][intrusive_forward_list]" )
//{
//	std::vector<test_type> objects = { 5, 4, 3, 2, 1 };
//	mclo::intrusive_forward_list<test_type> fl( objects );
//
//	fl.sort();
//
//	std::ranges::sort( objects );
//	CHECK_THAT( fl, RangeEquals( objects ) );
//}
//
//TEST_CASE( "mclo::intrusive_forward_list::sort - Random order list", "[intrusive][intrusive_forward_list]" )
//{
//	std::vector<test_type> objects = { 3, 1, 4, 5, 2 };
//	mclo::intrusive_forward_list<test_type> fl( objects );
//
//	fl.sort();
//
//	std::ranges::sort( objects );
//	CHECK_THAT( fl, RangeEquals( objects ) );
//}
//
//TEST_CASE( "mclo::intrusive_forward_list::sort - List with duplicates", "[intrusive][intrusive_forward_list]" )
//{
//	std::vector<test_type> objects = { 4, 2, 2, 3, 1, 4 };
//	mclo::intrusive_forward_list<test_type> fl(objects);
//
//	fl.sort();
//
//	std::ranges::sort( objects );
//	CHECK_THAT( fl, RangeEquals( objects ) );
//}
