#include <catch2/catch_test_macros.hpp>

#include "mclo/string/wide_convert.hpp"

TEST_CASE( "Convert UTF-8 string to wide string", "[string]" )
{
	const std::string utf8_str = "Hello, World!";
	const std::wstring wide_str = mclo::to_wstring( utf8_str );
	CHECK( wide_str == L"Hello, World!" );
}

TEST_CASE( "Convert empty UTF-8 string to wide string", "[string]" )
{
	const std::string utf8_str;
	const std::wstring wide_str = mclo::to_wstring( utf8_str );
	CHECK( wide_str == L"" );
}

TEST_CASE( "Convert wide string to UTF-8 string", "[string]" )
{
	const std::wstring wide_str = L"Hello, World!";
	const std::string utf8_str = mclo::from_wstring( wide_str );
	CHECK( utf8_str == "Hello, World!" );
}

TEST_CASE( "Convert empty wide string to UTF-8 string", "[string]" )
{
	const std::wstring wide_str;
	const std::string utf8_str = mclo::from_wstring( wide_str );
	CHECK( utf8_str == "" );
}

TEST_CASE( "Round-trip UTF-8 to wide and back", "[string]" )
{
	const std::string utf8_str = "Hello, World!";
	CHECK( mclo::from_wstring( mclo::to_wstring( utf8_str ) ) == utf8_str );
}
