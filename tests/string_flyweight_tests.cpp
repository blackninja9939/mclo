#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_aliases.hpp"
#include "mclo/string/string_buffer.hpp"
#include "mclo/string/string_flyweight.hpp"

namespace
{
	template <typename CharT>
	constexpr auto hello_world = mclo::trandscode_ascii_literal<CharT>( "hello world" );

	template <typename CharT>
	constexpr auto bye_world = mclo::trandscode_ascii_literal<CharT>( "goodbye world" );

	template <typename CharT>
	constexpr auto cool_string = mclo::trandscode_ascii_literal<CharT>( "new cool string" );
}

TEMPLATE_LIST_TEST_CASE( "string_flyweight default", "[string_flyweight]", mclo::meta::char_types )
{
	using test_string = mclo::immutable_string_flyweight<struct test_domain, TestType>;
	using view = typename test_string::view;
	const test_string default_handle;
	CHECK( default_handle == view() );
}

TEMPLATE_LIST_TEST_CASE( "string_flyweight with value", "[string_flyweight]", mclo::meta::char_types )
{
	using test_string = mclo::immutable_string_flyweight<struct test_domain, TestType>;
	using view = typename test_string::view;

	const test_string handle( hello_world<TestType> );
	CHECK( handle == view( hello_world<TestType> ) );

	const test_string handle2( hello_world<TestType> );
	CHECK( handle == handle2 );
	CHECK( handle2 == view( hello_world<TestType> ) );

	const test_string handle3( bye_world<TestType> );
	CHECK( handle != handle3 );

	// Existing handle still works
	CHECK( handle == view( hello_world<TestType> ) );
	CHECK( handle2 == view( hello_world<TestType> ) );

	// New handle is new string
	CHECK( handle3 == view( bye_world<TestType> ) );
}

TEMPLATE_LIST_TEST_CASE( "string_flyweight assign value", "[string_flyweight]", mclo::meta::char_types )
{
	using test_string = mclo::immutable_string_flyweight<struct test_domain, TestType>;
	using view = typename test_string::view;

	test_string handle( hello_world<TestType> );
	CHECK( handle == view( hello_world<TestType> ) );

	const test_string handle2( hello_world<TestType> );
	CHECK( handle == handle2 );
	CHECK( handle2 == view( hello_world<TestType> ) );

	handle = view( cool_string<TestType> );
	CHECK( handle != handle2 );
	CHECK( handle == view( cool_string<TestType> ) );
	CHECK( handle2 == view( hello_world<TestType> ) );
}

TEMPLATE_LIST_TEST_CASE( "string_flyweight swap", "[string_flyweight]", mclo::meta::char_types )
{
	using test_string = mclo::immutable_string_flyweight<struct test_domain, TestType>;
	using view = typename test_string::view;

	test_string handle( hello_world<TestType> );
	test_string handle2( bye_world<TestType> );
	CHECK( handle != handle2 );

	SECTION( "Member swap" )
	{
		handle.swap( handle2 );
		CHECK( handle == bye_world<TestType> );
		CHECK( handle2 == hello_world<TestType> );
	}
	SECTION( "Free swap" )
	{
		swap( handle, handle2 );
		CHECK( handle == bye_world<TestType> );
		CHECK( handle2 == hello_world<TestType> );
	}
}
