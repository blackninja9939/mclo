#include <catch2/catch_test_macros.hpp>

#include "mclo/string_buffer.hpp"
#include "mclo/string_buffer_io.hpp"

#include <sstream>

TEST_CASE( "string_buffer from literal", "[string_buffer]" )
{
	SECTION( "Compile time" )
	{
		constexpr mclo::basic_string_buffer str( "hello" );
		STATIC_CHECK( str == "hello" );
	}
	SECTION( "Run time" )
	{
		mclo::basic_string_buffer str( "hello" );
		CHECK( str == "hello" );
	}
}

TEST_CASE( "string_buffer ", "[string_buffer]" )
{
	mclo::string_buffer<64> str( "hello 123" );

	CHECK( str >= "hello" );
	CHECK( "hello" < str );
}

TEST_CASE( "string_buffer input streaming", "[string_buffer]" )
{
	std::istringstream stream( "hello to the world" );
	mclo::string_buffer<64> str;

	stream >> str;

	CHECK( str == "hello" );
}

TEST_CASE( "string_buffer output streaming", "[string_buffer]" )
{
	std::ostringstream stream;
	mclo::string_buffer<64> str( "hello to the world" );

	stream << str;

	CHECK( stream.str() == "hello to the world" );
}

#ifdef __cpp_lib_format
TEST_CASE( "string_buffer std::format", "[string_buffer]" )
{
	mclo::string_buffer<64> str( "hello to the world" );

	const std::string formatted = std::format( "{:-^22}", str );

	CHECK( formatted == "--hello to the world--" );
}
#endif

#ifdef __cpp_lib_starts_ends_with
TEST_CASE( "string_buffer starts_with", "[string_buffer]" )
{
	const mclo::string_buffer<64> str( "hello to the world" );

	// View
	std::string_view other = "hel";
	CHECK( str.starts_with( other ) );

	other.remove_prefix( 1 );
	CHECK( !str.starts_with( other ) );

	// Character
	CHECK( str.starts_with( 'h' ) );
	CHECK( !str.starts_with( 'H' ) );

	// Null terminated string
	CHECK( str.starts_with( "hell" ) );
	CHECK( !str.starts_with( "ello" ) );
}

TEST_CASE( "string_buffer ends_with", "[string_buffer]" )
{
	const mclo::string_buffer<64> str( "hello to the world" );

	// View
	std::string_view other = "the world";
	CHECK( str.ends_with( other ) );

	other.remove_suffix( 1 );
	CHECK( !str.ends_with( other ) );

	// Character
	CHECK( str.ends_with( 'd' ) );
	CHECK( !str.ends_with( 'D' ) );

	// Null terminated string
	CHECK( str.ends_with( "the world" ) );
	CHECK( !str.ends_with( "the worl" ) );
}
#endif

#ifdef __cpp_lib_string_contains
TEST_CASE( "string_buffer contains", "[string_buffer]" )
{
	const mclo::string_buffer<64> str( "hello to the world" );

	// View
	CHECK( str.contains( std::string_view( "the world" ) ) );
	CHECK( str.contains( std::string_view( "the orl" ) ) );
	CHECK( !str.contains( std::string_view( "yeet" ) ) );

	// Character
	CHECK( str.contains( 'h' ) );
	CHECK( !str.contains( 'D' ) );

	// Null terminated string
	CHECK( str.contains( "the world" ) );
	CHECK( str.contains( "he worl" ) );
	CHECK( !str.contains( "yeet" ) );
}
#endif
