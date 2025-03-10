#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/string/string_flyweight.hpp"

namespace
{
	using test_string = mclo::string_flyweight<struct test_domain>;
}

TEST_CASE( "string_flyweight default", "[string_flyweight]" )
{
	constexpr test_string default_handle;
	CHECK( default_handle == std::string_view() );
}

TEST_CASE( "string_flyweight with value", "[string_flyweight]" )
{
	using namespace std::string_view_literals;
	const test_string handle( "hello world" );
	CHECK( handle == "hello world"sv );

	const test_string handle2( "hello world" );
	CHECK( handle == handle2 );
	CHECK( handle2 == "hello world"sv );

	const test_string handle3( "goodbye world" );
	CHECK( handle != handle3 );

	// Existing handle still works
	CHECK( handle == "hello world"sv );
	CHECK( handle2 == "hello world"sv );

	// New handle is new string
	CHECK( handle3 == "goodbye world"sv );
}

TEST_CASE( "string_flyweight assign value", "[string_flyweight]" )
{
	using namespace std::string_view_literals;
	test_string handle( "hello world" );
	CHECK( handle == "hello world"sv );

	const test_string handle2( "hello world" );
	CHECK( handle == handle2 );
	CHECK( handle2 == "hello world"sv );

	handle = "new cool string"sv;
	CHECK( handle != handle2 );
	CHECK( handle == "new cool string"sv );
	CHECK( handle2 == "hello world"sv );
}

TEST_CASE( "string_flyweight swap", "[string_flyweight]" )
{
	using namespace std::string_view_literals;
	test_string handle( "hello world"sv );
	test_string handle2( "goodbye world"sv );
	CHECK( handle != handle2 );

	SECTION( "Member swap" )
	{
		handle.swap( handle2 );
		CHECK( handle == "goodbye world"sv );
		CHECK( handle2 == "hello world"sv );
	}
	SECTION( "Free swap" )
	{
		swap( handle, handle2 );
		CHECK( handle == "goodbye world"sv );
		CHECK( handle2 == "hello world"sv );
	}
}
