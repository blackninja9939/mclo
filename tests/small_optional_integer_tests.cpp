#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/meta.hpp"
#include "mclo/small_optional_integer.hpp"

template <typename T>
struct Catch::StringMaker<mclo::small_optional_integer<T>>
{
	static std::string convert( const mclo::small_optional_integer<T> value )
	{
		return value ? Catch::StringMaker<T>::convert( *value ) : "null";
	}
};

TEMPLATE_LIST_TEST_CASE( "small_optional_integer default", "[small_optional_integer]", mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> value;
	CHECK_FALSE( value.has_value() );
	CHECK_FALSE( value );
	CHECK( value.raw_value() == 0 );
	CHECK( value.value_or( 42 ) == 42 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer construct value", "[small_optional_integer]", mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> value{ 16 };
	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == 16 );
	CHECK( *value == 16 );
	CHECK( value.raw_value() == 17 );
	CHECK( value.value_or( 42 ) == 16 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer reset", "[small_optional_integer]", mclo::meta::integers )
{
	mclo::small_optional_integer<TestType> value{ 16 };
	value.reset();

	CHECK_FALSE( value.has_value() );
	CHECK_FALSE( value );
	CHECK( value.raw_value() == 0 );
	CHECK( value.value_or( 42 ) == 42 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer set", "[small_optional_integer]", mclo::meta::integers )
{
	mclo::small_optional_integer<TestType> value{ 16 };
	value.set( 8 );

	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == 8 );
	CHECK( *value == 8 );
	CHECK( value.raw_value() == 9 );
	CHECK( value.value_or( 42 ) == 8 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer signed", "[small_optional_integer]", mclo::meta::signed_integers )
{
	mclo::small_optional_integer<TestType> value{ -16 };
	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == -16 );
	CHECK( *value == -16 );
	CHECK( value.raw_value() == -16 );
	CHECK( value.value_or( 42 ) == -16 );

	value.set( 16 );
	CHECK( value.has_value() );
	CHECK( value );
	CHECK( value.value() == 16 );
	CHECK( *value == 16 );
	CHECK( value.raw_value() == 17 );
	CHECK( value.value_or( 42 ) == 16 );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer comparison empty", "[small_optional_integer]", mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> lhs;
	const mclo::small_optional_integer<TestType> rhs;
	CHECK( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK( lhs >= rhs );
	CHECK_FALSE( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK_FALSE( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer comparison lhs empty",
						 "[small_optional_integer]",
						 mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> lhs;
	const mclo::small_optional_integer<TestType> rhs{ 16 };
	CHECK_FALSE( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK_FALSE( lhs >= rhs );
	CHECK( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer comparison rhs empty",
						 "[small_optional_integer]",
						 mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> lhs{ 16 };
	const mclo::small_optional_integer<TestType> rhs;
	CHECK_FALSE( lhs == rhs );
	CHECK_FALSE( lhs <= rhs );
	CHECK( lhs >= rhs );
	CHECK_FALSE( lhs < rhs );
	CHECK( lhs > rhs );
	CHECK( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer comparison same value",
						 "[small_optional_integer]",
						 mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> lhs{ 16 };
	const mclo::small_optional_integer<TestType> rhs{ 16 };
	CHECK( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK( lhs >= rhs );
	CHECK_FALSE( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK_FALSE( lhs != rhs );
}

TEMPLATE_LIST_TEST_CASE( "small_optional_integer comparison different values",
						 "[small_optional_integer]",
						 mclo::meta::integers )
{
	const mclo::small_optional_integer<TestType> lhs{ 16 };
	const mclo::small_optional_integer<TestType> rhs{ 30 };
	CHECK_FALSE( lhs == rhs );
	CHECK( lhs <= rhs );
	CHECK_FALSE( lhs >= rhs );
	CHECK( lhs < rhs );
	CHECK_FALSE( lhs > rhs );
	CHECK( lhs != rhs );
}
