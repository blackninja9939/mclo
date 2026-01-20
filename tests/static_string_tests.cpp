#include <catch2/catch_test_macros.hpp>

#include "mclo/string/static_string.hpp"

TEST_CASE( "static_string default is empty", "[static_string]" )
{
	const mclo::static_string str;
	CHECK( str == str );
	CHECK_FALSE( str != str );
	CHECK( str.size() == 0 );
	CHECK_FALSE( str.data() );
	CHECK( std::string_view( str ).empty() );
}

TEST_CASE( "static_string equal to self", "[static_string]" )
{
	const mclo::static_string str( "Hello, World!" );
	CHECK( str == str );
	CHECK_FALSE( str != str );
}

TEST_CASE( "static_string from literal", "[static_string]" )
{
	const mclo::static_string str( "Hello, World!" );
	CHECK( std::string_view( str ) == "Hello, World!" );
	CHECK( str.size() == 13 );
	CHECK( str.data() );
}

TEST_CASE( "static_string from string view literal", "[static_string]" )
{
	using namespace std::string_view_literals;
	const mclo::static_string str( "Hello, World!"sv );
	CHECK( std::string_view( str ) == "Hello, World!"sv );
	CHECK( str.size() == 13 );
	CHECK( str.data() );
}

TEST_CASE( "static_string from compile time pointer", "[static_string]" )
{
	static constexpr const char* ptr = "Hello, World!";
	const mclo::static_string str( ptr );
	CHECK( std::string_view( str ) == ptr );
	CHECK( str.size() == 13 );
	CHECK( str.data() );
}

TEST_CASE( "static_string from compile time array", "[static_string]" )
{
	static constexpr const char array[] = "Hello, World!";
	const mclo::static_string str( array );
	CHECK( std::string_view( str ) == array );
	CHECK( str.size() == 13 );
	CHECK( str.data() );
}

TEST_CASE( "static_string from assumed static run time string", "[static_string]" )
{
	static const std::string runtime_str = "Hello, World!";
	const mclo::static_string str( mclo::assume_static, runtime_str );
	CHECK( std::string_view( str ) == runtime_str );
	CHECK( str.size() == 13 );
	CHECK( str.data() );
}

TEST_CASE( "static_string from assumed static run time literal", "[static_string]" )
{
	const char* literal = "Hello, World!";
	const mclo::static_string str( mclo::assume_static, literal );
	CHECK( std::string_view( str ) == literal );
	CHECK( str.size() == 13 );
	CHECK( str.data() );
}

TEST_CASE( "static_string hashes as stored string", "[static_string]" )
{
	const mclo::static_string str( "Hello, World!" );
	const std::string_view view = str;
	const std::hash<mclo::static_string> static_string_hasher;
	const std::hash<std::string_view> string_view_hasher;
	CHECK( static_string_hasher( str ) == string_view_hasher( view ) );
}
