#include <catch2/catch_test_macros.hpp>

#include "mclo/enum_map.hpp"

namespace
{
	enum class test_enum
	{
		first,
		second,
		third,
		fourth,
		count,
	};
}

TEST_CASE( "enum_map default constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map;
	CHECK( map.size() == 4 );
	CHECK( map.front() == 0 );
	CHECK( map.back() == 0 );
	CHECK( map.data() );

	for ( const int& value : map )
	{
		CHECK( value == 0 );
	}
	for ( const int& value : map | std::views::reverse )
	{
		CHECK( value == 0 );
	}

	std::size_t count = 0;
	for ( const auto& [ index, value ] : map.enumerate() )
	{
		CHECK( value == 0 );
		CHECK( static_cast<test_enum>( count ) == index );
		++count;
	}
}

TEST_CASE( "enum_map fill constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map{ 4 };
	CHECK( map.size() == 4 );
	CHECK( map.front() == 4 );
	CHECK( map.back() == 4 );
	CHECK( map.data() );

	for ( const int& value : map )
	{
		CHECK( value == 4 );
	}
}

TEST_CASE( "enum_map variadic constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map{ 1, 2, 3, 4 };
	CHECK( map.size() == 4 );
	CHECK( map.front() == 1 );
	CHECK( map.back() == 4 );
	CHECK( map.data() );

	int index = 1;
	for ( const int& value : map )
	{
		CHECK( value == index );
		++index;
	}
}

TEST_CASE( "enum_map enumerate", "[enum_map]" )
{
	mclo::enum_map<test_enum, int> map;

	int count = 0;
	for ( auto [ index, value ] : map.enumerate() )
	{
		CHECK( value == 0 );
		CHECK( static_cast<test_enum>( count ) == index );
		value = count + 10;
		++count;
	}

	count = 0;
	for ( const auto [ index, value ] : map.enumerate() )
	{
		CHECK( value == static_cast<int>( index ) + 10 );
		CHECK( static_cast<test_enum>( count ) == index );
		++count;
	}
}

TEST_CASE( "enum_map fill", "[enum_map]" )
{
	mclo::enum_map<test_enum, int> map;
	CHECK( map.size() == 4 );
	CHECK( map.front() == 0 );
	CHECK( map.back() == 0 );
	CHECK( map.data() );

	map.fill( 8 );

	CHECK( map.front() == 8 );
	CHECK( map.back() == 8 );

	for ( const int& value : map )
	{
		CHECK( value == 8 );
	}
}

TEST_CASE( "enum_map index with key", "[enum_map]" )
{
	mclo::enum_map<test_enum, int> map;
	map[ test_enum::first ] = 8;
	map[ test_enum::third ] = 42;

	CHECK( map.front() == 8 );
	CHECK( map.back() == 0 );

	const auto& const_map = std::as_const( map );
	CHECK( const_map[ test_enum::first ] == 8 );
	CHECK( const_map[ test_enum::third ] == 42 );
}

TEST_CASE( "enum_map index directly", "[enum_map]" )
{
	mclo::enum_map<test_enum, int> map;
	map.index_direct( 0 ) = 8;
	map.index_direct( 2 ) = 42;

	CHECK( map.front() == 8 );
	CHECK( map.back() == 0 );

	const auto& const_map = std::as_const( map );
	CHECK( const_map.index_direct( 0 ) == 8 );
	CHECK( const_map.index_direct( 2 ) == 42 );

	CHECK( const_map[ test_enum::first ] == 8 );
	CHECK( const_map[ test_enum::third ] == 42 );
}
