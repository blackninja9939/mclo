#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/container/circular_buffer.hpp"

#include <array>
#include <unordered_set>

using namespace Catch::Matchers;

namespace
{
	void check_empty( const mclo::circular_buffer<int>& buffer )
	{
		CHECK( buffer.empty() );
		CHECK( buffer.size() == 0 );
		CHECK( buffer.begin() == buffer.end() );
		CHECK( buffer.cbegin() == buffer.cend() );
		CHECK( buffer.rbegin() == buffer.rend() );
		CHECK( buffer.crbegin() == buffer.crend() );
		CHECK( std::distance( buffer.begin(), buffer.end() ) == 0 );
		CHECK( std::distance( buffer.cbegin(), buffer.cend() ) == 0 );
		CHECK_THAT( buffer, RangeEquals( std::array<int, 0>{} ) );
		const auto [ first, second ] = buffer.as_contiguous();
		CHECK( first.empty() );
		CHECK( second.empty() );
	}

	void check_equals( const mclo::circular_buffer<int>& buffer, mclo::span<const int> values )
	{
		DEBUG_ASSERT( !values.empty() );
		CHECK_FALSE( buffer.empty() );
		CHECK( buffer.size() == values.size() );
		CHECK( buffer.begin() != buffer.end() );
		CHECK( buffer.cbegin() != buffer.cend() );
		CHECK( buffer.rbegin() != buffer.rend() );
		CHECK( buffer.crbegin() != buffer.crend() );
		CHECK( std::distance( buffer.begin(), buffer.end() ) == std::ranges::distance( values ) );
		CHECK( std::distance( buffer.cbegin(), buffer.cend() ) == std::ranges::distance( values ) );
		CHECK_THAT( buffer, RangeEquals( values ) );
		const auto [ first, second ] = buffer.as_contiguous();
		CHECK_FALSE( ( first.empty() && second.empty() ) );
		std::unordered_set<int> set;
		set.insert( first.begin(), first.end() );
		set.insert( second.begin(), second.end() );
		CHECK_THAT( set, UnorderedRangeEquals( values ) );
	}
}

TEST_CASE( "circular_buffer default construct is empty no capacity", "[circular_buffer]" )
{
	mclo::circular_buffer<int> buffer;
	CHECK( buffer.capacity() == 0 );
	check_empty( buffer );
}

TEST_CASE( "circular_buffer construct with capacity is empty", "[circular_buffer]" )
{
	mclo::circular_buffer<int> buffer( 5 );
	CHECK( buffer.capacity() == 5 );
	check_empty( buffer );
}

TEST_CASE( "circular_buffer push back contains value", "[circular_buffer]" )
{
	mclo::circular_buffer<int> buffer( 5 );
	buffer.push_back( 1 );

	check_equals( buffer, { 1 } );
}

TEST_CASE( "circular_buffer push front contains value", "[circular_buffer]" )
{
	mclo::circular_buffer<int> buffer( 5 );
	buffer.push_front( 1 );

	check_equals( buffer, { 1 } );
}

TEST_CASE( "circular_buffer push back over capacity overwrites", "[circular_buffer]" )
{
	mclo::circular_buffer<int> buffer( 5 );
	for ( int i = 0; i <= buffer.capacity(); ++i )
	{
		buffer.push_back( i );
	}

	check_equals( buffer, { 1, 2, 3, 4, 5 } );
}

//TEST_CASE( "Quick test" )
//{
//	mclo::circular_buffer<int> buffer( 5 );
//	buffer.push_front( 1 );
//	buffer.push_front( 2 );
//
//	buffer.push_back( 3 );
//	buffer.push_back( 3 );
//	buffer.push_back( 3 );
//	buffer.push_back( 3 );
//	buffer.push_front( 3 );
//
//	{
//		const auto [ first, second ] = buffer.as_contiguous();
//		CHECK_FALSE( first.empty() );
//		CHECK_FALSE( second.empty() );
//	}
//
//	buffer.make_contiguous();
//
//	{
//		const auto [ first, second ] = buffer.as_contiguous();
//		CHECK_FALSE( first.empty() );
//		CHECK( second.empty() );
//	}
//}
