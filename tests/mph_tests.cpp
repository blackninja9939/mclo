#if __cplusplus >= 202002L

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_aliases.hpp"
#include "mclo/container/mph.hpp"

namespace
{
	template <typename T>
	constexpr T make( const std::string_view value ) = delete;

	template <>
	constexpr std::string_view make( const std::string_view value )
	{
		return value;
	}

	template <>
	constexpr int make( const std::string_view value )
	{
		int result = 0;
		for ( const char c : value )
		{
			result *= 10;
			result += c - '0';
		}
		return result;
	}

	template <>
	constexpr std::optional<int> make( const std::string_view value )
	{
		return make<int>( value );
	}

	using test_types = mclo::meta::type_list<int, std::optional<int>, std::string_view>;
}

TEMPLATE_LIST_TEST_CASE( "mph_map::find", "[mph]", test_types )
{
	static constexpr auto mk = &make<TestType>;
	static constexpr mclo::mph_map<TestType, char, 3> map{
		{ { { mk( "42" ), 'h' }, { mk( "109" ), 'g' }, { mk( "-32" ), 'j' } } } };
	static constexpr auto it = map.find( mk( "42" ) );
	STATIC_REQUIRE( it != map.end() );
	STATIC_CHECK( it->first == mk( "42" ) );
	STATIC_CHECK( it->second == 'h' );
}

TEMPLATE_LIST_TEST_CASE( "mph_map::contains", "[mph]", test_types )
{
	static constexpr auto mk = &make<TestType>;
	static constexpr mclo::mph_map<TestType, char, 3> map{
		{ { { mk( "42" ), 'h' }, { mk( "109" ), 'g' }, { mk( "-32" ), 'j' } } } };
	STATIC_CHECK( map.contains( mk( "109" ) ) );
}

TEMPLATE_LIST_TEST_CASE( "mph_set::find", "[mph]", test_types )
{
	static constexpr auto mk = &make<TestType>;
	static constexpr mclo::mph_set<TestType, 3> set{ { { mk( "42" ), mk( "109" ), mk( "-32" ) } } };
	static constexpr auto it = set.find( mk( "42" ) );
	STATIC_REQUIRE( it != set.end() );
	STATIC_CHECK( *it == mk( "42" ) );
}

TEMPLATE_LIST_TEST_CASE( "mph_set::contains", "[mph]", test_types )
{
	static constexpr auto mk = &make<TestType>;
	static constexpr mclo::mph_set<TestType, 3> set{ { { mk( "42" ), mk( "109" ), mk( "-32" ) } } };
	STATIC_CHECK( set.contains( mk( "109" ) ) );
}

auto foo()
{
	static constexpr mclo::mph_hash<std::optional<int>> hash;
	static constexpr std::optional<int> i;
	static constexpr std::size_t ret = hash( i, 4 );
	return ret;
}

#endif
