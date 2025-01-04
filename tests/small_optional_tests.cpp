#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta/type_aliases.hpp"
#include "mclo/utility/small_optional.hpp"

template <typename T>
struct Catch::StringMaker<mclo::small_optional<T>>
{
	static std::string convert( const mclo::small_optional<T> value )
	{
		return value ? Catch::StringMaker<T>::convert( *value ) : "null";
	}
};

TEMPLATE_LIST_TEST_CASE( "small_optional default", "[small_optional]", mclo::meta::integers )
{
	const mclo::small_optional<TestType> value;
	CHECK_FALSE( value.has_value() );
	CHECK_FALSE( value );
	CHECK( value.value_or( 42 ) == 42 );

	CHECK_THROWS_AS( value.value(), std::bad_optional_access );
}

TEMPLATE_LIST_TEST_CASE( "small_optional construct value", "[small_optional]", mclo::meta::integers )
{
	const mclo::small_optional<TestType> value{ 16 };
	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == 16 );
	CHECK( *value == 16 );
	CHECK( value.value_or( 42 ) == 16 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional reset", "[small_optional]", mclo::meta::integers )
{
	mclo::small_optional<TestType> value{ 16 };
	value.reset();

	CHECK_FALSE( value.has_value() );
	CHECK_FALSE( value );
	CHECK( value.value_or( 42 ) == 42 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional set", "[small_optional]", mclo::meta::integers )
{
	mclo::small_optional<TestType> value{ 16 };
	value.set( 8 );

	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == 8 );
	CHECK( *value == 8 );
	CHECK( value.value_or( 42 ) == 8 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional signed", "[small_optional]", mclo::meta::signed_integers )
{
	mclo::small_optional<TestType> value{ -16 };
	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == -16 );
	CHECK( *value == -16 );
	CHECK( value.value_or( 42 ) == -16 );

	value.set( 16 );
	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == 16 );
	CHECK( *value == 16 );
	CHECK( value.value_or( 42 ) == 16 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional comparison empty", "[small_optional]", mclo::meta::integers )
{
	const mclo::small_optional<TestType> lhs;
	const mclo::small_optional<TestType> rhs;
	CHECK( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK( lhs >= rhs );
	CHECK_FALSE( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK_FALSE( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional comparison lhs empty",
						 "[small_optional]",
						 mclo::meta::integers )
{
	const mclo::small_optional<TestType> lhs;
	const mclo::small_optional<TestType> rhs{ 16 };
	CHECK_FALSE( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK_FALSE( lhs >= rhs );
	CHECK( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional comparison rhs empty",
						 "[small_optional]",
						 mclo::meta::integers )
{
	const mclo::small_optional<TestType> lhs{ 16 };
	const mclo::small_optional<TestType> rhs;
	CHECK_FALSE( lhs == rhs );
	CHECK_FALSE( lhs <= rhs );
	CHECK( lhs >= rhs );
	CHECK_FALSE( lhs < rhs );
	CHECK( lhs > rhs );
	CHECK( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional comparison same value",
						 "[small_optional]",
						 mclo::meta::integers )
{
	const mclo::small_optional<TestType> lhs{ 16 };
	const mclo::small_optional<TestType> rhs{ 16 };
	CHECK( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK( lhs >= rhs );
	CHECK_FALSE( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK_FALSE( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional comparison different values",
						 "[small_optional]",
						 mclo::meta::integers )
{
	const mclo::small_optional<TestType> lhs{ 16 };
	const mclo::small_optional<TestType> rhs{ 30 };
	CHECK_FALSE( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK_FALSE( lhs >= rhs );
	CHECK( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional hash", "[small_optional]", mclo::meta::integers )
{
	const mclo::small_optional<TestType> empty;
	const mclo::small_optional<TestType> opt{ 16 };
	using hash = std::hash<mclo::small_optional<TestType>>;
	CHECK( hash{}( empty ) != hash{}( opt ) );
}

TEST_CASE( "small_optional bool", "[small_optional]" )
{
	const mclo::small_optional<bool> empty;
	const mclo::small_optional<bool> set_true{ true };
	const mclo::small_optional<bool> set_false{ false };
	CHECK_FALSE( empty );
}

TEST_CASE( "small_optional pointer", "[small_optional]" )
{
	int i = 2;
	const mclo::small_optional<int*> empty;
	const mclo::small_optional<int*> set{ &i };
	CHECK_FALSE( empty );
}

enum class checker
{
	a,
	b,
	c,
	d,
	e,
	f
};

TEST_CASE( "small_optional enum", "[small_optional]" )
{
	const mclo::small_optional<checker> empty;
	const mclo::small_optional<checker> set{ checker::e };
	CHECK_FALSE( empty );
}

TEMPLATE_LIST_TEST_CASE( "small_optional float", "[small_optional]", mclo::meta::floating_points )
{
	const mclo::small_optional<TestType> empty;
	const mclo::small_optional<TestType> set{ 2.42f };
	CHECK_FALSE( empty );
}

TEST_CASE( "small_optional string_view", "[small_optional]" )
{
	const mclo::small_optional<std::string_view> empty;
	const mclo::small_optional<std::string_view> set{ "hello" };
	CHECK_FALSE( empty );
}
