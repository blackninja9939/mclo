#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/container/hash_map.hpp"
#include "mclo/hash/std_types.hpp"

#include "mclo/container/span.hpp"

TEST_CASE( "Hash map" )
{
	mclo::hash_map<std::string, int> map;
	map.emplace( "hello", 5 );

	auto it = map.find( "hello" );
	REQUIRE( it != std::as_const( map ).end() );
	CHECK( it->first == "hello");

	CHECK( it->second == 5 );

	for ( const auto& pair : map )
	{
		CHECK(pair.first == "hello");
	}

	map.emplace( "hello", 4 );
	map.emplace( "helloo", 4 );
	map.emplace( "hellooo", 4 );
	map.emplace( "hellooooo", 4 );

	map.erase( "hello" );
	
	for ( const auto& pair : map )
	{
		CHECK( pair.second == 4 );
	}

	mclo::hash_map<std::string, int> map2 = std::move( map );
}
