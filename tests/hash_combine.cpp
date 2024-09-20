#include <catch2/catch_test_macros.hpp>

#include "mclo/hash_combine.hpp"

#include <array>

TEST_CASE( "hash_combine", "[hash]" )
{
	const std::size_t a = std::hash<int>()( 42 );
	const std::size_t b = std::hash<int>()( -42 );
	CHECK( a != b );

	const std::size_t combined = mclo::hash_combine( 42, -42 );
	CHECK( a != combined );
}

TEST_CASE( "hash_range iterators", "[hash]" )
{
	const std::array<int, 2> first{ 42, 43 };
	const std::array<int, 2> second{ 42, 44 };
	CHECK( mclo::hash_range( first.begin(), first.end() ) != mclo::hash_range( second.begin(), second.end() ) );
}

TEST_CASE( "hash_range range", "[hash]" )
{
	const std::array<int, 2> first{ 42, 43 };
	const std::array<int, 2> second{ 42, 44 };
	CHECK( mclo::hash_range( first ) != mclo::hash_range( second ) );
}

TEST_CASE( "hash_range range and iterators same", "[hash]" )
{
	const std::array<int, 2> arr{ 42, 43 };
	CHECK( mclo::hash_range( arr.begin(), arr.end() ) == mclo::hash_range( arr ) );
}
