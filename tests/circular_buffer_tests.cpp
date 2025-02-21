#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "mclo/container/circular_buffer.hpp"

#include <array>

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

TEST_CASE( "circular_buffer construct with capacity and fill is full", "[circular_buffer]" )
{
	mclo::circular_buffer<int> buffer( 5 );
	buffer.push_back( 1 );

	CHECK( buffer.capacity() == 5 );
	CHECK( !buffer.empty() );
	CHECK( buffer.size() == 1 );
	CHECK( buffer.front() == 1 );
	CHECK( buffer.back() == 1 );
	CHECK( buffer.begin() != buffer.end() );
	CHECK( buffer.cbegin() != buffer.cend() );
	CHECK( buffer.rbegin() != buffer.rend() );
	CHECK( buffer.crbegin() != buffer.crend() );
	CHECK( std::distance( buffer.begin(), buffer.end() ) == 1 );
	CHECK( std::distance( buffer.cbegin(), buffer.cend() ) == 1 );
	CHECK_THAT( buffer, RangeEquals( std::array{ 1 } ) );
	CHECK_THAT( buffer.as_contiguous().first, RangeEquals( std::array{ 1 } ) );
}

TEST_CASE( "Quick test" )
{
	mclo::circular_buffer<int> buffer( 5 );
	buffer.push_front( 1 );
	buffer.push_front( 2 );

	buffer.push_back( 3 );
	buffer.push_back( 3 );
	buffer.push_back( 3 );
	buffer.push_back( 3 );
	buffer.push_front( 3 );

	{
		const auto [ first, second ] = buffer.as_contiguous();
		CHECK_FALSE( first.empty() );
		CHECK_FALSE( second.empty() );
	}

	buffer.make_contiguous();

	{
		const auto [ first, second ] = buffer.as_contiguous();
		CHECK_FALSE( first.empty() );
		CHECK( second.empty() );
	}
}
