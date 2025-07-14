#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/lazy_convert_construct.hpp"

#include <unordered_map>

namespace
{
	int make_10()
	{
		return 10;
	}
}

TEST_CASE( "lazy_convert_construct constructible from function and calls function", "[utility]" )
{
	const mclo::lazy_convert_construct lazy = &make_10;
	static_assert( std::is_convertible_v<decltype( lazy ), int> );

	const int value = lazy;
	CHECK( value == 10 );
}

TEST_CASE( "lazy_convert_construct constructible from lambda and calls lambda", "[utility]" )
{
	const mclo::lazy_convert_construct lazy = [] { return 42; };
	static_assert( std::is_convertible_v<decltype( lazy ), int> );
	const int value = lazy;
	CHECK( value == 42 );
}

TEST_CASE( "lazy_convert_construct constructible from lambda with capture and calls lambda", "[utility]" )
{
	int value = 0;
	const mclo::lazy_convert_construct lazy = [ &value ] { return ++value; };
	static_assert( std::is_convertible_v<decltype( lazy ), int> );
	CHECK( value == 0 );

	const int result = lazy;
	CHECK( result == 1 );
	const int result2 = lazy;
	CHECK( result2 == 2 );
	CHECK( value == 2 );
}

TEST_CASE( "lazy_convert_construct used with unordered_map::try_emplace calls lazy only on success insert",
		   "[utility]" )
{
	int calls = 0;
	std::unordered_map<int, int> map;

	const auto [ it, inserted ] = map.try_emplace( 1, mclo::lazy_convert_construct{ [ &calls ] {
													   calls++;
													   return 42;
												   } } );
	CHECK( inserted );
	CHECK( it->second == 42 );
	CHECK( calls == 1 );

	const auto [ it2, inserted2 ] = map.try_emplace( 1, mclo::lazy_convert_construct{ [ &calls ] {
														 calls++;
														 return 100;
													 } } );
	CHECK_FALSE( inserted2 );
	CHECK( it2->second == 42 );
	CHECK( calls == 1 );

	const auto [ it3, inserted3 ] = map.try_emplace( 2, mclo::lazy_convert_construct{ [ &calls ] {
														 calls++;
														 return 100;
													 } } );
	CHECK( inserted3 );
	CHECK( it3->second == 100 );
	CHECK( calls == 2 );
}
