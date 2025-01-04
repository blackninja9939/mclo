#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/array.hpp"

namespace
{
	constexpr int broadcast_value = 42;

	template <typename T, std::size_t size>
	constexpr bool are_equal( const std::array<T, size>& lhs, const std::array<T, size>& rhs )
	{
		return std::equal( lhs.begin(), lhs.end(), rhs.begin() );
	}
}

TEST_CASE( "broadcast_array", "[array]" )
{
	constexpr std::array<int, 5> manual{
		broadcast_value, broadcast_value, broadcast_value, broadcast_value, broadcast_value };

	constexpr std::array arr = mclo::broadcast_array<5>( broadcast_value );
	STATIC_CHECK( are_equal( arr, manual ) );
}

TEST_CASE( "join_arrays", "[array]" )
{
	constexpr std::array first{ 1, 2, 3, 4, 5 };
	constexpr std::array second{ 5, 4, 3, 2, 1 };

	SECTION( "compile time" )
	{
		constexpr std::array joined = mclo::join_arrays( first, second );
		STATIC_CHECK( are_equal( joined, std::array{ 1, 2, 3, 4, 5, 5, 4, 3, 2, 1 } ) );
	}
	SECTION( "run time" )
	{
		const std::array joined = mclo::join_arrays( first, second );
		CHECK( are_equal( joined, std::array{ 1, 2, 3, 4, 5, 5, 4, 3, 2, 1 } ) );
	}
}
