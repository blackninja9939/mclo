#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/try.hpp"

namespace
{
	std::optional<bool> test_optional( const std::optional<std::string>& value )
	{
		MCLO_TRY( const std::string& result, value );
		CHECK( result == "hello" );
		return true;
	}

	std::optional<bool> test_optional( std::optional<std::string>&& value )
	{
		MCLO_TRY( std::string result, std::move( value ) );
		CHECK( result == "hello" );
		CHECK( result != *value );
		return true;
	}

	mclo::expected<bool, int> test_expected( const mclo::expected<std::string, int>& value )
	{
		MCLO_TRY( const std::string& result, value );
		CHECK( result == "hello" );
		return true;
	}

	mclo::expected<bool, int> test_expected( mclo::expected<std::string, int>&& value )
	{
		MCLO_TRY( std::string result, std::move( value ) );
		CHECK( result == "hello" );
		CHECK( result != *value );
		return true;
	}

	mclo::expected<void, int> test_expected( const mclo::expected<void, int>& value )
	{
		MCLO_TRY_VOID( value );
		return {};
	}
}

TEST_CASE( "try_traits optional", "[try]" )
{
	const std::optional<int> value = 42;
	CHECK( mclo::try_traits<std::optional<int>>::has_value( value ) );
	CHECK( mclo::try_traits<std::optional<int>>::value( value ) == 42 );
}

TEST_CASE( "try_traits empty optional", "[try]" )
{
	const std::optional<int> value;
	CHECK_FALSE( mclo::try_traits<std::optional<int>>::has_value( value ) );
}

TEST_CASE( "try_traits engaged expected", "[try]" )
{
	const mclo::expected<int, int> value = 42;
	CHECK( mclo::try_traits<mclo::expected<int, int>>::has_value( value ) );
	CHECK( mclo::try_traits<mclo::expected<int, int>>::value( value ) == 42 );
}

TEST_CASE( "try_traits error expected", "[try]" )
{
	const mclo::expected<int, int> value = mclo::unexpected( 1337 );
	CHECK_FALSE( mclo::try_traits<mclo::expected<int, int>>::has_value( value ) );
	CHECK( mclo::try_traits<mclo::expected<int, int>>::error( value ) == mclo::unexpected( 1337 ) );
}

TEST_CASE( "try_traits void expected", "[try]" )
{
	const mclo::expected<void, int> value;
	CHECK( mclo::try_traits<mclo::expected<void, int>>::has_value( value ) );
}

TEST_CASE( "MCLO_TRY optional success", "[try]" )
{
	const std::optional<std::string> value = "hello";
	const std::optional<bool> result = test_optional( value );
	REQUIRE( result );
	CHECK( *result );
}

TEST_CASE( "MCLO_TRY optional error", "[try]" )
{
	const std::optional<std::string> value = std::nullopt;
	const std::optional<bool> result = test_optional( value );
	CHECK_FALSE( result );
}

TEST_CASE( "MCLO_TRY expected success", "[try]" )
{
	const mclo::expected<std::string, int> value = "hello";
	const mclo::expected<bool, int> result = test_expected( value );
	REQUIRE( result );
	CHECK( *result );
}

TEST_CASE( "MCLO_TRY expected success void", "[try]" )
{
	const mclo::expected<void, int> value;
	const mclo::expected<void, int> result = test_expected( value );
	REQUIRE( result );
}

TEST_CASE( "MCLO_TRY expected error", "[try]" )
{
	const mclo::expected<std::string, int> value = mclo::unexpected( 42 );
	const mclo::expected<bool, int> result = test_expected( value );
	CHECK_FALSE( result );
	CHECK( result.error() == 42 );
}

TEST_CASE( "MCLO_TRY expected error void", "[try]" )
{
	const mclo::expected<void, int> value = mclo::unexpected( 42 );
	const mclo::expected<void, int> result = test_expected( value );
	CHECK_FALSE( result );
	CHECK( result.error() == 42 );
}

TEST_CASE( "MCLO_TRY optional success rvalue", "[try]" )
{
	std::optional<std::string> value = "hello";
	const std::optional<bool> result = test_optional( std::move( value ) );
	REQUIRE( result );
	CHECK( *result );
}

TEST_CASE( "MCLO_TRY optional error rvalue", "[try]" )
{
	std::optional<std::string> value = std::nullopt;
	const std::optional<bool> result = test_optional( std::move( value ) );
	CHECK_FALSE( result );
}

TEST_CASE( "MCLO_TRY expected success rvalue", "[try]" )
{
	mclo::expected<std::string, int> value = "hello";
	const mclo::expected<bool, int> result = test_expected( std::move( value ) );
	REQUIRE( result );
	CHECK( *result );
}

TEST_CASE( "MCLO_TRY expected error rvalue", "[try]" )
{
	mclo::expected<std::string, int> value = mclo::unexpected( 42 );
	const mclo::expected<bool, int> result = test_expected( std::move( value ) );
	CHECK_FALSE( result );
	CHECK( result.error() == 42 );
}
