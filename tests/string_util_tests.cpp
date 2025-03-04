#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "consteval_check.hpp"
#include "mclo/meta/type_aliases.hpp"
#include "mclo/string//hash.hpp"
#include "mclo/string/compare_ignore_case.hpp"
#include "mclo/string/string_buffer.hpp"
#include "mclo/string/string_utils.hpp"
#include "mclo/string/trim.hpp"
#include "mclo/string/join.hpp"
// todo(mc) split up these test files

#include <array>
#include <iterator>
#include <sstream>
#include <string>

using mclo::meta::char_types;
using mclo::meta::integers;

TEMPLATE_LIST_TEST_CASE( "from_string", "[string]", integers )
{
	auto result = mclo::from_string<TestType>( "42" );
	REQUIRE( result );
	CHECK( *result == 42 );
}

TEMPLATE_LIST_TEST_CASE( "to_string std::array buffer", "[string]", integers )
{
	std::array<char, 64> buffer;
	const std::string_view result = mclo::to_string( buffer, TestType( 42 ) );
	CHECK( result == "42" );
}

TEMPLATE_LIST_TEST_CASE( "to_string c-buffer", "[string]", integers )
{
	char buffer[ 64 ];
	const std::string_view result = mclo::to_string( buffer, TestType( 42 ) );
	CHECK( result == "42" );
}

TEMPLATE_LIST_TEST_CASE( "to_string std::string", "[string]", integers )
{
	std::string buffer( 64, '\0' );
	const std::string_view result = mclo::to_string( buffer.data(), buffer.data() + buffer.size(), TestType( 42 ) );
	CHECK( result == "42" );
}

namespace
{
	constexpr std::string_view lorem_ipsum_mixed =
		"LoReM IpSuM DoLoR SiT AmEt, CoNsEcTeTuR AdIpIsCiNg eLiT. vEsTiBuLuM NoN FeLiS.";

	constexpr std::string_view lorem_ipsum_lower =
		"lorem ipsum dolor sit amet, consectetur adipiscing elit. vestibulum non felis.";

	constexpr std::string_view lorem_ipsum_upper =
		"LOREM IPSUM DOLOR SIT AMET, CONSECTETUR ADIPISCING ELIT. VESTIBULUM NON FELIS.";
}

TEST_CASE( "to_lower", "[string]" )
{
	std::string string( lorem_ipsum_mixed );

	SECTION( "Container" )
	{
		mclo::to_lower( string );
		CHECK( string == lorem_ipsum_lower );
	}
	SECTION( "Iterators" )
	{
		mclo::to_lower( string.begin(), string.end() );
		CHECK( string == lorem_ipsum_lower );
	}
	SECTION( "Pointers" )
	{
		mclo::to_lower( string.data(), string.data() + string.size() );
		CHECK( string == lorem_ipsum_lower );
	}
}

TEST_CASE( "to_lower compile time", "[string]" )
{
	SECTION( "Container" )
	{
		constexpr auto string = []() constexpr {
			mclo::string_buffer<128> string( lorem_ipsum_mixed );
			mclo::to_lower( string );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == lorem_ipsum_lower );
	}
	SECTION( "Iterators" )
	{
		constexpr auto string = []() constexpr {
			mclo::string_buffer<128> string( lorem_ipsum_mixed );
			mclo::to_lower( string.begin(), string.end() );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == lorem_ipsum_lower );
	}
	SECTION( "Pointers" )
	{
		constexpr auto string = []() constexpr {
			mclo::string_buffer<128> string( lorem_ipsum_mixed );
			mclo::to_lower( string.data(), string.data() + string.size() );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == lorem_ipsum_lower );
	}
}

TEST_CASE( "to_upper", "[string]" )
{
	std::string string( lorem_ipsum_mixed );

	SECTION( "Container" )
	{
		mclo::to_upper( string );
		CHECK( string == lorem_ipsum_upper );
	}
	SECTION( "Iterators" )
	{
		mclo::to_upper( string.begin(), string.end() );
		CHECK( string == lorem_ipsum_upper );
	}
	SECTION( "Pointers" )
	{
		mclo::to_upper( string.data(), string.data() + string.size() );
		CHECK( string == lorem_ipsum_upper );
	}
}

TEST_CASE( "to_upper compile time", "[string]" )
{
	SECTION( "Container" )
	{
		constexpr auto string = []() constexpr {
			mclo::string_buffer<128> string( lorem_ipsum_mixed );
			mclo::to_upper( string );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == lorem_ipsum_upper );
	}
	SECTION( "Iterators" )
	{
		constexpr auto string = []() constexpr {
			mclo::string_buffer<128> string( lorem_ipsum_mixed );
			mclo::to_upper( string.begin(), string.end() );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == lorem_ipsum_upper );
	}
	SECTION( "Pointers" )
	{
		constexpr auto string = []() constexpr {
			mclo::string_buffer<128> string( lorem_ipsum_mixed );
			mclo::to_upper( string.data(), string.data() + string.size() );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == lorem_ipsum_upper );
	}
}

TEMPLATE_LIST_TEST_CASE( "trim_front", "[string]", char_types )
{
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( "  \n leading spaces\n  " );
		constexpr auto trimmed = mclo::trim_front( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "leading spaces\n  " ) );
	}
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( "no leading spaces\n  " );
		constexpr auto trimmed = mclo::trim_front( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "no leading spaces\n  " ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "trim_back", "[string]", char_types )
{
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( "  \n trailing spaces  \n" );
		constexpr auto trimmed = mclo::trim_back( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "  \n trailing spaces" ) );
	}
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( "  \n no trailing spaces" );
		constexpr auto trimmed = mclo::trim_back( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "  \n no trailing spaces" ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "trim", "[string]", char_types )
{
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( " \n  all spaces   \n" );
		constexpr auto trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "all spaces" ) );
	}
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( "no leading spaces  \n " );
		constexpr auto trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "no leading spaces" ) );
	}
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( " \n   no trailing spaces" );
		constexpr auto trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "no trailing spaces" ) );
	}
	{
		static constexpr auto string = mclo::trandscode_ascii_literal<TestType>( "no spaces" );
		constexpr auto trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == mclo::trandscode_ascii_literal<TestType>( "no spaces" ) );
	}
}

TEST_CASE( "compare_ignore_case", "[string]" )
{
	{
		constexpr std::string_view upper = "45 HELLO WORLD! 123!!!!!!!!!!!!!!!!!!!!!!";
		constexpr std::string_view lower = "45 hello world! 123!!!!!!!!!!!!!!!!!!!!!!";
		CONSTEVAL_CHECK( mclo::compare_ignore_case( upper, lower ) == 0 );
	}
	{
		constexpr std::string_view upper = "HELLO WORLD!!!!!!!!!!!!!!!!!!!!!!!";
		constexpr std::string_view lower = "hello world?!!!!!!!!!!!!!!!!!!!!!!";
		CONSTEVAL_CHECK( mclo::compare_ignore_case( upper, lower ) < 0 );
		CONSTEVAL_CHECK( mclo::compare_ignore_case( lower, upper ) > 0 );
	}
	{
		constexpr std::string_view upper = "YELLO WORLD!!!!!!!!!!!!!!!!!!!!!!";
		constexpr std::string_view lower = "hello world!!!!!!!!!!!!!!!!!!!!!!";
		CONSTEVAL_CHECK( mclo::compare_ignore_case( upper, lower ) > 0 );
		CONSTEVAL_CHECK( mclo::compare_ignore_case( lower, upper ) < 0 );
	}
}

TEMPLATE_LIST_TEST_CASE( "replace_all", "[string]", char_types )
{
	constexpr auto initial = mclo::trandscode_ascii_literal<TestType>( "1.2.14.9" );
	constexpr auto what = mclo::trandscode_ascii_literal<TestType>( "." );
	constexpr auto with = mclo::trandscode_ascii_literal<TestType>( ".." );
	constexpr auto result = mclo::trandscode_ascii_literal<TestType>( "1..2..14..9" );
	std::basic_string<TestType> string( initial );
	mclo::replace_all( string, what, with );
	CHECK( string == result );
}

TEST_CASE( "join_string variadic", "[string]" )
{
	using namespace std::literals;
	const std::string result = mclo::join_string( "hello"s, " to "sv, "the world" );
	CHECK( result == "hello to the world" );
}

TEMPLATE_TEST_CASE( "join_string iterators", "[string]", std::string, std::string_view, const char* )
{
	const std::array<TestType, 3> arr{ "hello", " to ", "the world" };
	const std::string result = mclo::join_string( arr.rbegin(), arr.rend() );
	CHECK( result == "the world to hello" );
}

TEST_CASE( "join_string input iterators", "[string]" )
{
	std::istringstream str( "hello to the world" );
	const std::string result =
		mclo::join_string( std::istreambuf_iterator<char>( str ), std::istreambuf_iterator<char>() );
	CHECK( result == "hello to the world" );
}

TEMPLATE_TEST_CASE( "join_string container", "[string]", std::string, std::string_view, const char* )
{
	const std::array<TestType, 3> arr{ "hello", " to ", "the world" };
	const std::string result = mclo::join_string( arr );
	CHECK( result == "hello to the world" );
}

TEMPLATE_TEST_CASE( "join_string container constexpr", "[string]", std::string_view, const char* )
{
	constexpr std::array<TestType, 3> arr{ "hello", " to ", "the world" };
	constexpr auto result = mclo::join_string<mclo::string_buffer<32>>( arr );
	STATIC_CHECK( std::string_view( result ) == "hello to the world" );
}

TEST_CASE( "join_string char* iterators", "[string]" )
{
	const std::array<const char*, 3> arr{ "hello", " to ", "the world" };
	const std::string result_out_buffer = mclo::join_string<std::string, 2>( arr.rbegin(), arr.rend() );
	const std::string result_in_buffer = mclo::join_string<std::string, 6>( arr.rbegin(), arr.rend() );

	CHECK( result_out_buffer == "the world to hello" );
	CHECK( result_in_buffer == result_out_buffer );
}

TEMPLATE_LIST_TEST_CASE( "string_hash", "[string][hash]", char_types )
{
	CONSTEVAL_CHECK( mclo::string_hash( mclo::trandscode_ascii_literal<TestType>( "hello" ) ) !=
					 mclo::string_hash( mclo::trandscode_ascii_literal<TestType>( "Hello" ) ) );
}

TEST_CASE( "string_hash string types same hash", "[string][hash]" )
{
	using namespace std::literals;
	const std::size_t literal_hash = mclo::string_hash( "hello" );
	const char* ptr = "hello";
	CHECK( literal_hash == mclo::string_hash( ptr ) );
	CHECK( literal_hash == mclo::string_hash( "hello"sv ) );
	CHECK( literal_hash == mclo::string_hash( "hello"s ) );
}

TEST_CASE( "string_hash_ignore_case", "[string][hash]" )
{
	CONSTEVAL_CHECK( mclo::string_hash_ignore_case( "hello" ) == mclo::string_hash_ignore_case( "Hello" ) );
}
