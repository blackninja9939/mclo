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
