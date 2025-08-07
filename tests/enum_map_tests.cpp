#include <catch2/catch_test_macros.hpp>

#include "mclo/enum/enum_map.hpp"

namespace
{
	enum class test_enum
	{
		first,
		second,
		third,
		fourth,
		enum_size,
	};

	static_assert( std::ranges::random_access_range<mclo::enum_map<test_enum, int>> );
}

TEST_CASE( "enum_map default constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map;
	CHECK( map.size() == 4 );
	CHECK( map.front() == 0 );
	CHECK( map.back() == 0 );

	std::size_t count = 0;
	for ( const auto& [ key, value ] : map )
	{
		CHECK( value == 0 );
		CHECK( static_cast<test_enum>( count ) == key );
		++count;
	}
}

TEST_CASE( "enum_map fill constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map{ 4 };
	CHECK( map.size() == 4 );
	CHECK( map.front() == 4 );
	CHECK( map.back() == 4 );

	for ( const int& value : map.as_span() )
	{
		CHECK( value == 4 );
	}
}

TEST_CASE( "enum_map variadic value initializer list constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map{ 1, 2, 3, 4 };
	CHECK( map.size() == 4 );
	CHECK( map.front() == 1 );
	CHECK( map.back() == 4 );

	int index = 1;
	for ( const int& value : map.as_span() )
	{
		CHECK( value == index );
		++index;
	}
}

TEST_CASE( "enum_map range constructor", "[enum_map]" )
{
	constexpr std::array<std::pair<test_enum, int>, 6> data{
		{
         { test_enum::first, 8 },
         { test_enum::fourth, 4 },
         { test_enum::first, -9 },
         { test_enum::third, 3 },
         { test_enum::first, 1 },
         { test_enum::second, 2 },
		 }
    };
	const mclo::enum_map<test_enum, int> map{ data };
	CHECK( map.size() == 4 );
	CHECK( map.front() == 1 );
	CHECK( map.back() == 4 );

	int index = 1;
	for ( const int& value : map.as_span() )
	{
		CHECK( value == index );
		++index;
	}
}

TEST_CASE( "enum_map init list constructor", "[enum_map]" )
{
	const mclo::enum_map<test_enum, int> map{
		{ test_enum::first,  8},
		{test_enum::fourth,  4},
		{ test_enum::first, -9},
		{ test_enum::third,  3},
		{ test_enum::first,  1},
		{test_enum::second,  2},
	};
	CHECK( map.size() == 4 );
	CHECK( map.front() == 1 );
	CHECK( map.back() == 4 );

	int index = 1;
	for ( const int& value : map.as_span() )
	{
		CHECK( value == index );
		++index;
	}
}

TEST_CASE( "enum_map iterate", "[enum_map]" )
{
	mclo::enum_map<test_enum, int> map;

	int count = 0;
	for ( auto [ index, value ] : map )
	{
		CHECK( value == 0 );
		CHECK( static_cast<test_enum>( count ) == index );
		value = count + 10;
		++count;
	}

	count = 0;
	for ( const auto [ index, value ] : map )
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

	map.fill( 8 );

	CHECK( map.front() == 8 );
	CHECK( map.back() == 8 );

	for ( const int& value : map.as_span() )
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

TEST_CASE( "enum_map narrowing conversions", "[enum_map]" )
{
	const mclo::enum_map<test_enum, std::uint8_t> map{ 1, 2, 3, 4 };
}

#include "mclo/enum/enum_string_bi_map.hpp"
TEST_CASE( "BiMap" )
{
	std::array<std::pair<test_enum, std::string_view>, 4> arr{
		{ { test_enum::first, "first" },
         { test_enum::second, "second" },
         { test_enum::third, "third" },
         { test_enum::fourth, "fourth" } }
    };
	mclo::enum_string_bi_map<test_enum> map( arr );

	const auto result = map.lookup_from_string( "second" );
	REQUIRE( result );
	CHECK( *result == test_enum::second );
}
