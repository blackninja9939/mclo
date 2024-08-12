#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_aliases.hpp"
#include "mclo/string/string_table.hpp"

namespace
{
	struct test_domain;

	template <typename IndexType>
	using test_table = mclo::string_table<test_domain, IndexType>;
}

TEMPLATE_LIST_TEST_CASE( "string_table default", "[string_table]", mclo::meta::unsigned_integers )
{
	constexpr typename test_table<TestType>::handle default_handle;
	test_table<TestType> table;
	CHECK( table.size() == 0 );
	CHECK_FALSE( table.lookup_handle( "hello world" ).is_valid() );
	CHECK( table.lookup_handle( "hello world" ) == default_handle );
}

TEMPLATE_LIST_TEST_CASE( "string_table insert", "[string_table]", mclo::meta::unsigned_integers )
{
	test_table<TestType> table;
	const auto handle = table.insert( "hello world" );
	CHECK( handle.is_valid() );
	CHECK( table.lookup_string( handle ) == "hello world" );
	CHECK( table.lookup_handle( "hello world" ) == handle );

	const auto handle2 = table.insert( "hello world" );
	CHECK( handle == handle2 );
	CHECK( handle2.is_valid() );
	CHECK( table.lookup_string( handle2 ) == "hello world" );
	CHECK( table.lookup_handle( "hello world" ) == handle2 );

	const auto handle3 = table.insert( "goodbye world" );
	CHECK( handle != handle3 );

	// Existing handle still works
	CHECK( handle.is_valid() );
	CHECK( table.lookup_string( handle ) == "hello world" );
	CHECK( table.lookup_handle( "hello world" ) == handle );

	// New handle is new string
	CHECK( handle3.is_valid() );
	CHECK( table.lookup_string( handle3 ) == "goodbye world" );
	CHECK( table.lookup_handle( "goodbye world" ) == handle3 );
}
