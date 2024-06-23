#include <catch2/catch_test_macros.hpp>

#include "mclo/hash_combine.hpp"

TEST_CASE( "hash_combine", "[hash]" )
{
	const std::size_t a = std::hash<int>()( 42 );
	const std::size_t b = std::hash<int>()( -42 );
	CHECK( a != b );

	const std::size_t combined = mclo::hash_combine( 42, -42 );
	CHECK( a != combined );
}
