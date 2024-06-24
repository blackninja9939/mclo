#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta.hpp"
#include "mclo/string_buffer.hpp"
#include "mclo/string_utils.hpp"

#include <array>
#include <iterator>
#include <sstream>
#include <string>

namespace
{
	using numeric_types = mclo::meta::type_list<signed char,
												unsigned char,
												short,
												unsigned short,
												int,
												unsigned int,
												long,
												unsigned long,
												long long,
												unsigned long long>;

}

TEMPLATE_LIST_TEST_CASE( "from_string", "[string]", numeric_types )
{
	auto result = mclo::from_string<TestType>( "42" );
	REQUIRE( result );
	CHECK( *result == 42 );
}

TEMPLATE_LIST_TEST_CASE( "to_string std::array buffer", "[string]", numeric_types )
{
	std::array<char, 64> buffer;
	const std::string_view result = mclo::to_string( buffer, TestType( 42 ) );
	CHECK( result == "42" );
}

TEMPLATE_LIST_TEST_CASE( "to_string c-buffer", "[string]", numeric_types )
{
	char buffer[ 64 ];
	const std::string_view result = mclo::to_string( buffer, TestType( 42 ) );
	CHECK( result == "42" );
}

TEMPLATE_LIST_TEST_CASE( "to_string std::string", "[string]", numeric_types )
{
	std::string buffer( 64, '\0' );
	const std::string_view result = mclo::to_string( buffer.data(), buffer.data() + buffer.size(), TestType( 42 ) );
	CHECK( result == "42" );
}

TEST_CASE( "to_lower", "[string]" )
{
	SECTION( "Run time" )
	{
		std::string string = "HELlo woRld";
		mclo::to_lower( string );
		CHECK( string == "hello world" );
	}
	SECTION( "Compile time" )
	{
		constexpr mclo::string_buffer<32> string = []() constexpr {
			mclo::string_buffer<32> string = "HELlo woRld";
			mclo::to_lower( string.rbegin(), string.rend() );
			return string;
		}();
		STATIC_CHECK( std::string_view( string ) == "hello world" );
	}
}

TEST_CASE( "trim_front", "[string]" )
{
	{
		constexpr std::string_view string = "  \n leading spaces\n  ";
		constexpr std::string_view trimmed = mclo::trim_front( string );
		STATIC_CHECK( trimmed == "leading spaces\n  " );
	}
	{
		constexpr std::string_view string = "no leading spaces\n  ";
		constexpr std::string_view trimmed = mclo::trim_front( string );
		STATIC_CHECK( trimmed == "no leading spaces\n  " );
	}
}

TEST_CASE( "trim_back", "[string]" )
{
	{
		constexpr std::string_view string = "  \n trailing spaces  \n";
		constexpr std::string_view trimmed = mclo::trim_back( string );
		STATIC_CHECK( trimmed == "  \n trailing spaces" );
	}
	{
		constexpr std::string_view string = "  \n no trailing spaces";
		constexpr std::string_view trimmed = mclo::trim_back( string );
		STATIC_CHECK( trimmed == "  \n no trailing spaces" );
	}
}

TEST_CASE( "trim", "[string]" )
{
	{
		constexpr std::string_view string = " \n  all spaces   \n";
		constexpr std::string_view trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == "all spaces" );
	}
	{
		constexpr std::string_view string = "no leading spaces  \n ";
		constexpr std::string_view trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == "no leading spaces" );
	}
	{
		constexpr std::string_view string = " \n   no trailing spaces";
		constexpr std::string_view trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == "no trailing spaces" );
	}
	{
		constexpr std::string_view string = "no spaces";
		constexpr std::string_view trimmed = mclo::trim( string );
		STATIC_CHECK( trimmed == "no spaces" );
	}
}

TEST_CASE( "compare_ignore_case", "[string]" )
{
	{
		constexpr std::string_view upper = "45 HELLO WORLD! 123";
		constexpr std::string_view lower = "45 hello world! 123";
		STATIC_CHECK( mclo::compare_ignore_case( upper, lower ) == 0 );
	}
	{
		constexpr std::string_view upper = "HELLO WORLD";
		constexpr std::string_view lower = "hello world!";
		STATIC_CHECK( mclo::compare_ignore_case( upper, lower ) < 0 );
		STATIC_CHECK( mclo::compare_ignore_case( lower, upper ) > 0 );
	}
	{
		constexpr std::string_view upper = "YELLO WORLD";
		constexpr std::string_view lower = "hello world";
		STATIC_CHECK( mclo::compare_ignore_case( upper, lower ) > 0 );
		STATIC_CHECK( mclo::compare_ignore_case( lower, upper ) < 0 );
	}
}

template <typename CharT>
struct replace_test_data
{
	using string = std::basic_string<CharT>;
	static constexpr std::basic_string_view<CharT> initial = "1.2.14.9";
	static constexpr std::basic_string_view<CharT> what = ".";
	static constexpr std::basic_string_view<CharT> with = "..";
	static constexpr std::basic_string_view<CharT> result = "1..2..14..9";
};

template <>
struct replace_test_data<wchar_t>
{
	using string = std::wstring;
	static constexpr std::wstring_view initial = L"1.2.14.9";
	static constexpr std::wstring_view what = L".";
	static constexpr std::wstring_view with = L"..";
	static constexpr std::wstring_view result = L"1..2..14..9";
};

TEMPLATE_TEST_CASE( "replace_all", "[string]", char, wchar_t )
{
	using trait = replace_test_data<TestType>;
	typename trait::string string( trait::initial );
	mclo::replace_all( string, trait::what, trait::with );
	CHECK( string == trait::result );
}

TEST_CASE( "join_string variadic", "[string]" )
{
	using namespace std::literals;
	const std::string result = mclo::join_string( "hello"s, " to "sv, "the world" );
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

TEST_CASE( "string_hash", "[string][hash]" )
{
	using namespace std::literals;
	STATIC_CHECK( mclo::string_hash( "hello" ) != mclo::string_hash( "Hello" ) );
	STATIC_CHECK( mclo::string_hash_ignore_case( "hello" ) == mclo::string_hash_ignore_case( "Hello" ) );

	CHECK( mclo::string_hash( "hello" ) != mclo::string_hash( "Hello" ) );
	CHECK( mclo::string_hash_ignore_case( "hello" ) == mclo::string_hash_ignore_case( "Hello" ) );
}
