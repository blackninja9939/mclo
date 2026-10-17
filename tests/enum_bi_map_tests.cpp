#include "consteval_check.hpp"

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/enum/enum_bi_map.hpp"
#include "mclo/meta/type_list.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

namespace
{
	enum class test_enum : std::uint8_t
	{
		first,
		second,
		third,
		fourth,
		enum_size,
	};

	using enum_bi_maps = mclo::meta::type_list<
		mclo::enum_bi_map_linear<test_enum, int>,
		mclo::enum_bi_map_binary<test_enum, int>,
		mclo::enum_bi_map<test_enum, int>>;

	enum class large_enum : std::uint8_t
	{
		enum_size = 20,
	};
}

TEST_CASE( "BiMap" )
{
	std::array<std::pair<test_enum, std::string_view>, 4> arr{ {
		{ test_enum::first, "first" },
		{ test_enum::second, "second" },
		{ test_enum::third, "third" },
		{ test_enum::fourth, "fourth" },
	} };
	mclo::enum_bi_map<test_enum, std::string_view> map( arr );

	const auto result = map.lookup_from_data( "second" );
	REQUIRE( result );
	CHECK( *result == test_enum::second );
}

TEMPLATE_LIST_TEST_CASE(
	"enum_bi_map with unsorted values, looking up keys, returns only exact matches", "[enum_bi_map]", enum_bi_maps
)
{
	static constexpr std::array<std::pair<test_enum, int>, 4> data{ {
		{ test_enum::third, 30 },
		{ test_enum::first, 10 },
		{ test_enum::fourth, 40 },
		{ test_enum::second, 20 },
	} };
	static constexpr TestType map( data );

	CONSTEVAL_CHECK( map.lookup_from_data( 10 ) == test_enum::first );
	CONSTEVAL_CHECK( map.lookup_from_data( 20 ) == test_enum::second );
	CONSTEVAL_CHECK( map.lookup_from_data( 30 ) == test_enum::third );
	CONSTEVAL_CHECK( map.lookup_from_data( 40 ) == test_enum::fourth );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 9 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 11 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 29 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 39 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 41 ).has_value() );

	CONSTEVAL_CHECK( map.lookup_from_enum( test_enum::first ) == 10 );
	CONSTEVAL_CHECK( map.lookup_from_enum( test_enum::second ) == 20 );
	CONSTEVAL_CHECK( map.lookup_from_enum( test_enum::third ) == 30 );
	CONSTEVAL_CHECK( map.lookup_from_enum( test_enum::fourth ) == 40 );
}

TEST_CASE( "enum_bi_map with twenty entries, looking up absent keys, returns no enumerator", "[enum_bi_map]" )
{
	static constexpr auto data = [] {
		std::array<std::pair<large_enum, int>, 20> entries{};
		for ( std::size_t index = 0; index < entries.size(); ++index )
		{
			entries[ index ] = { static_cast<large_enum>( index ), 10 + static_cast<int>( index ) * 2 };
		}
		return entries;
	}();
	static constexpr mclo::enum_bi_map<large_enum, int> map( data );

	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 9 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 11 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 47 ).has_value() );
	CONSTEVAL_CHECK_FALSE( map.lookup_from_data( 49 ).has_value() );

	for ( const auto& [ key, value ] : data )
	{
		CHECK( map.lookup_from_data( value ) == key );
		CHECK( map.lookup_from_enum( key ) == value );
	}
}
