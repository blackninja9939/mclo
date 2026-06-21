#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_list.hpp"

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

#include "mclo/utility/map_lookup.hpp"

namespace
{
	using test_types = mclo::meta::type_list<std::unordered_map<int, std::string>, std::map<int, std::string>>;
}

TEMPLATE_LIST_TEST_CASE( "lookup_value_or with present key returns value", "[map_lookup]", test_types )
{
	TestType map;
	map[ 1 ] = "found";

	const auto result = mclo::lookup_value_or( map, 1, "default" );

	CHECK( result == "found" );
}

TEMPLATE_LIST_TEST_CASE( "lookup_value_or with absent key returns default", "[map_lookup]", test_types )
{
	TestType map;
	map[ 1 ] = "found";

	const auto result = mclo::lookup_value_or( map, 42, "default" );

	CHECK( result == "default" );
}

TEMPLATE_LIST_TEST_CASE( "lookup_ref_or with present key returns reference to value", "[map_lookup]", test_types )
{
	// Verify that the deleted rvalue overload prevents passing a temporary as the default value
	static_assert(
		!requires( const TestType& m, const typename TestType::key_type& k ) {
			mclo::lookup_ref_or( m, k, typename TestType::mapped_type{} );
		}, "lookup_ref_or should not accept an rvalue default" );

	TestType map;
	const auto& mapValue = map[ 1 ] = "found";

	const std::string defaultVal = "default";
	const auto& result = mclo::lookup_ref_or( map, 1, defaultVal );

	CHECK( result == "found" );
	CHECK( &mapValue == &result );
}

TEMPLATE_LIST_TEST_CASE( "lookup_ref_or with absent key returns reference to default", "[map_lookup]", test_types )
{
	TestType map;
	map[ 1 ] = "found";

	const std::string defaultVal = "default";
	const auto& result = mclo::lookup_ref_or( map, 42, defaultVal );

	CHECK( result == "default" );
	CHECK( &defaultVal == &result );
}

TEMPLATE_LIST_TEST_CASE( "lookup on empty map returns default", "[map_lookup]", test_types )
{
	const TestType map;

	CHECK( mclo::lookup_value_or( map, 1, "default" ) == "default" );

	const std::string defaultVal = "default";
	const auto& result = mclo::lookup_ref_or( map, 1, defaultVal );
	CHECK( result == "default" );
	CHECK( &defaultVal == &result );
}

TEST_CASE( "lookup with heterogeneous key uses transparent comparator", "[map_lookup]" )
{
	std::map<std::string, std::string, std::less<>> map;
	const auto& mapValue = map[ "key" ] = "found";

	using namespace std::string_view_literals;
	constexpr auto presentKey = "key"sv;
	constexpr auto absentKey = "missing"sv;

	CHECK( mclo::lookup_value_or( map, presentKey, "default" ) == "found" );
	CHECK( mclo::lookup_value_or( map, absentKey, "default" ) == "default" );

	const std::string defaultVal = "default";
	CHECK( &mclo::lookup_ref_or( map, presentKey, defaultVal ) == &mapValue );
	CHECK( &mclo::lookup_ref_or( map, absentKey, defaultVal ) == &defaultVal );
}
