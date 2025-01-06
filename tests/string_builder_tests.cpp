#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_list.hpp"
#include "mclo/string/string_builder.hpp"

#include <string>

using namespace std::literals;

namespace
{
	using test_types = mclo::meta::type_list<mclo::string_builder, mclo::fixed_string_builder<256>>;
}

TEMPLATE_LIST_TEST_CASE( "string_builder append", "[string_builder]", test_types )
{
	TestType builder;
	builder.append( 'a' );
	builder.append( "bc" );
	builder.append( "de"s );
	builder.append( "fg"sv );
	builder.append( true );
	builder.append( false );
	builder.append( 42 );
	builder.append( 42u );
	builder.append( 42l );
	builder.append( 42ul );
	builder.append( 42ll );
	builder.append( 42ull );
	builder.append( 42.1f );
	builder.append( 42.1 );
	builder.append( 42.1l );
	CHECK( builder.view() == "abcdefgtruefalse42424242424242.142.142.1" );
}

TEMPLATE_LIST_TEST_CASE( "string_builder clear", "[string_builder]", test_types )
{
	TestType builder;
	builder.append( "abc" );
	builder.clear();
	builder.append( "def" );
	CHECK( builder.view() == "def" );
}

TEMPLATE_LIST_TEST_CASE( "string_builder c_str", "[string_builder]", test_types )
{
	TestType builder;
	builder.append( "abc" );
	const char* str = builder.c_str();
	REQUIRE( str );
	CHECK( std::strlen( str ) == 3 );
	CHECK( std::string_view( str ) == "abc" );
}

TEMPLATE_LIST_TEST_CASE( "string_builder to_string", "[string_builder]", test_types )
{
	TestType builder;
	builder.append( "abc" );
	const std::string str = builder.to_string();
	CHECK( str == "abc" );

	builder.append( "def" );
	CHECK( str == "abc" );
	CHECK( builder.to_string() == "abcdef" );
}
