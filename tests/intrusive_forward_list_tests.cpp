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

TEST_CASE( "default constructed intrusive_forward_list, is empty", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;
	CHECK( list.empty() );
	CHECK( std::ranges::distance( list ) == 0 );
}

TEST_CASE( "intrusive_forward_list constructed from iterator pair, contains elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };

	const mclo::intrusive_forward_list<test_type> list( objects.begin(), objects.end() );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "intrusive_forward_list constructed from range, contains elements", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };

	const mclo::intrusive_forward_list<test_type> list( objects );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "empty intrusive_forward_list, pop_front, returns nullptr", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;

	const test_type* const ptr = list.pop_front();

	CHECK_FALSE( ptr );
}

TEST_CASE( "empty intrusive_forward_list, push_front, contains element", "[intrusive][intrusive_forward_list]" )
{
	test_type object{ 3 };
	mclo::intrusive_forward_list<test_type> list;

	list.push_front( object );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 1 );
	CHECK_THAT( list, RangeEquals( std::array{ object } ) );
}

TEST_CASE( "empty intrusive_forward_list, push_front multiple, contains elements in reverse insertion order",
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

TEST_CASE( "non-empty intrusive_forward_list, pop_front, removes first element", "[intrusive][intrusive_forward_list]" )
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

TEST_CASE( "non-empty intrusive_forward_list, clear, is empty", "[intrusive][intrusive_forward_list]" )
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

TEST_CASE( "empty intrusive_forward_list, insert_after before_begin, contains element",
		   "[intrusive][intrusive_forward_list]" )
{
	test_type object{ 3 };
	mclo::intrusive_forward_list<test_type> list;

	list.insert_after( list.before_begin(), object );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 1 );
	CHECK_THAT( list, RangeEquals( std::array{ object } ) );
}

TEST_CASE( "non-empty intrusive_forward_list, insert_after begin, inserts after begin",
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

TEST_CASE( "empty intrusive_forward_list, insert_after range after before_begin, contains elements in order",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list;

	list.insert_after( list.before_begin(), objects.begin(), objects.end() );

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "non-empty intrusive_forward_list, insert_after range, inserts after pos",
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

TEST_CASE( "non-empty intrusive_forward_list, reverse, contains reversed elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 0, 1, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list( objects );

	list.reverse();

	CHECK_FALSE( list.empty() );
	CHECK( std::ranges::distance( list ) == 5 );
	CHECK_THAT( list, RangeEquals( objects | std::views::reverse ) );
}

TEST_CASE( "two intrusive_forward_lists, splice_after entire list, moves all elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> fl1( objects.begin(), objects.begin() + 3 );
	mclo::intrusive_forward_list<test_type> fl2( objects.begin() + 3, objects.end() );

	fl1.splice_after( fl1.before_begin(), fl2 );

	CHECK( fl2.empty() );
	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 4, 5, 6, 1, 2, 3 } ) );
}

TEST_CASE( "two intrusive_forward_lists, splice_after single element, moves one element",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> fl1( objects.begin(), objects.begin() + 3 );
	mclo::intrusive_forward_list<test_type> fl2( objects.begin() + 3, objects.end() );

	auto it = fl2.before_begin();
	fl1.splice_after( fl1.before_begin(), fl2, it );

	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 4, 1, 2, 3 } ) );
	CHECK_THAT( fl2, RangeEquals( std::vector<test_type>{ 5, 6 } ) );
}

TEST_CASE( "two intrusive_forward_lists, splice_after range, moves range of elements",
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

TEST_CASE( "empty and non-empty intrusive_forward_list, splice_after entire list, moves all elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> fl1;
	mclo::intrusive_forward_list<test_type> fl2( objects );

	fl1.splice_after( fl1.before_begin(), fl2 );

	CHECK( fl2.empty() );
	CHECK_THAT( fl1, RangeEquals( std::vector<test_type>{ 4, 5, 6 } ) );
}

TEST_CASE( "intrusive_forward_list, splice_after onto self, no change", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };
	mclo::intrusive_forward_list<test_type> fl;
	fl.assign( objects.begin(), objects.end() );

	auto it = fl.before_begin();
	fl.splice_after( it, fl, it );
	CHECK_THAT( fl, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5 } ) );
}

TEST_CASE( "intrusive_forward_list with no duplicates, unique, unchanged", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 2, 3, 4, 5 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 0 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "intrusive_forward_list with all elements identical, unique, removes all but one",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 1, 1, 1, 1 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 4 );
	CHECK_THAT( list, RangeEquals( std::vector<test_type>{ 1 } ) );
}

TEST_CASE( "intrusive_forward_list with consecutive duplicates, unique, removes duplicates",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 1, 2, 2, 3, 3, 3, 4, 5, 5 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 5 );
	CHECK_THAT( list, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5 } ) );
}

TEST_CASE( "intrusive_forward_list with non-consecutive duplicates, unique, unchanged",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects{ 1, 2, 1, 3, 2, 3, 4 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.unique();

	CHECK( removed == 0 );
	CHECK_THAT( list, RangeEquals( objects ) );
}

TEST_CASE( "empty intrusive_forward_list, unique, unchanged", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> list;

	const std::size_t removed = list.unique();

	CHECK( removed == 0ull );
	CHECK( list.empty() );
}

TEST_CASE( "already sorted intrusive_forward_list, sort, remains sorted", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };
	mclo::intrusive_forward_list<test_type> fl( objects );

	fl.sort();

	CHECK_THAT( fl, RangeEquals( objects ) );
}

TEST_CASE( "reverse sorted intrusive_forward_list, sort, is sorted ascending", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 5, 4, 3, 2, 1 };
	mclo::intrusive_forward_list<test_type> fl( objects );

	fl.sort();

	CHECK_THAT( fl, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5 } ) );
}

TEST_CASE( "randomly ordered intrusive_forward_list, sort, is sorted ascending", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 3, 1, 4, 5, 2 };
	mclo::intrusive_forward_list<test_type> fl( objects );

	fl.sort();

	CHECK_THAT( fl, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5 } ) );
}

TEST_CASE( "intrusive_forward_list with duplicates, sort, is sorted ascending", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 4, 2, 2, 3, 1, 4 };
	mclo::intrusive_forward_list<test_type> fl( objects );

	fl.sort();

	CHECK_THAT( fl, RangeEquals( std::vector<test_type>{ 1, 2, 2, 3, 4, 4 } ) );
}

TEST_CASE( "empty and single element intrusive_forward_list, sort, unchanged", "[intrusive][intrusive_forward_list]" )
{
	mclo::intrusive_forward_list<test_type> empty;
	empty.sort();
	CHECK( empty.empty() );

	std::vector<test_type> objects = { 42 };
	mclo::intrusive_forward_list<test_type> single( objects );
	single.sort();
	CHECK_THAT( single, RangeEquals( objects ) );
}

TEST_CASE( "intrusive_forward_list, sort with custom comparator, is sorted descending",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 3, 1, 4, 5, 2 };
	mclo::intrusive_forward_list<test_type> fl( objects );

	fl.sort( std::greater<>{} );

	CHECK_THAT( fl, RangeEquals( std::vector<test_type>{ 5, 4, 3, 2, 1 } ) );
}

TEST_CASE( "two sorted intrusive_forward_lists, merge, contains all elements in order",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> lhs_objects = { 1, 3, 5, 7 };
	std::vector<test_type> rhs_objects = { 2, 4, 6, 8 };
	mclo::intrusive_forward_list<test_type> lhs( lhs_objects );
	mclo::intrusive_forward_list<test_type> rhs( rhs_objects );

	lhs.merge( rhs );

	CHECK( rhs.empty() );
	CHECK_THAT( lhs, RangeEquals( std::vector<test_type>{ 1, 2, 3, 4, 5, 6, 7, 8 } ) );
}

TEST_CASE( "intrusive_forward_list, merge with empty operands, preserves elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3 };

	mclo::intrusive_forward_list<test_type> into_empty;
	mclo::intrusive_forward_list<test_type> source( objects );
	into_empty.merge( source );
	CHECK( source.empty() );
	CHECK_THAT( into_empty, RangeEquals( objects ) );

	mclo::intrusive_forward_list<test_type> from_empty;
	into_empty.merge( from_empty );
	CHECK_THAT( into_empty, RangeEquals( objects ) );
}

TEST_CASE( "two sorted intrusive_forward_lists of uneven length with duplicates, merge, contains all elements in order",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> lhs_objects = { 1, 2, 2, 9 };
	std::vector<test_type> rhs_objects = { 2, 3 };
	mclo::intrusive_forward_list<test_type> lhs( lhs_objects );
	mclo::intrusive_forward_list<test_type> rhs( rhs_objects );

	lhs.merge( rhs );

	CHECK( rhs.empty() );
	CHECK_THAT( lhs, RangeEquals( std::vector<test_type>{ 1, 2, 2, 2, 3, 9 } ) );
}

TEST_CASE( "non-empty intrusive_forward_list, move construct, transfers elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5 };
	mclo::intrusive_forward_list<test_type> source( objects );

	mclo::intrusive_forward_list<test_type> moved( std::move( source ) );

	CHECK( source.empty() );
	CHECK_THAT( moved, RangeEquals( objects ) );
}

TEST_CASE( "non-empty intrusive_forward_list, move assign, transfers elements", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> source_objects = { 1, 2, 3 };
	std::vector<test_type> dest_objects = { 9, 8 };
	mclo::intrusive_forward_list<test_type> source( source_objects );
	mclo::intrusive_forward_list<test_type> dest( dest_objects );

	dest = std::move( source );

	CHECK( source.empty() );
	CHECK_THAT( dest, RangeEquals( source_objects ) );
}

TEST_CASE( "two intrusive_forward_lists, swap, exchanges contents", "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> lhs_objects = { 1, 2, 3 };
	std::vector<test_type> rhs_objects = { 7, 8 };
	mclo::intrusive_forward_list<test_type> lhs( lhs_objects );
	mclo::intrusive_forward_list<test_type> rhs( rhs_objects );

	lhs.swap( rhs );

	CHECK_THAT( lhs, RangeEquals( rhs_objects ) );
	CHECK_THAT( rhs, RangeEquals( lhs_objects ) );

	using std::swap;
	swap( lhs, rhs );

	CHECK_THAT( lhs, RangeEquals( lhs_objects ) );
	CHECK_THAT( rhs, RangeEquals( rhs_objects ) );
}

TEST_CASE( "intrusive_forward_list with matching elements, remove, erases matching elements",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 2, 4, 2 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.remove( test_type{ 2 } );

	CHECK( removed == 3 );
	CHECK_THAT( list, RangeEquals( std::vector<test_type>{ 1, 3, 4 } ) );
}

TEST_CASE( "intrusive_forward_list, remove_if, erases elements matching predicate",
		   "[intrusive][intrusive_forward_list]" )
{
	std::vector<test_type> objects = { 1, 2, 3, 4, 5, 6 };
	mclo::intrusive_forward_list<test_type> list( objects );

	const std::size_t removed = list.remove_if( []( const test_type& value ) noexcept { return value.i % 2 == 0; } );

	CHECK( removed == 3 );
	CHECK_THAT( list, RangeEquals( std::vector<test_type>{ 1, 3, 5 } ) );
}
