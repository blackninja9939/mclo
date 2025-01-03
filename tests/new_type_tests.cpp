#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/new_type.hpp"
#include "mclo/meta/type_list.hpp"

template <typename T>
consteval bool shared_static_asserts() noexcept
{
	using value_type = typename T::value_type;
	static_assert( std::is_nothrow_convertible_v<T&, const value_type&> );
	static_assert( std::is_nothrow_convertible_v<const T&, const value_type&> );
	static_assert( std::three_way_comparable<T> );
	return true;
}

using trivial_new_type = mclo::new_type<int, struct trivial_new_type_tag>;

static_assert( std::is_trivially_default_constructible_v<trivial_new_type> );
static_assert( std::is_constructible_v<trivial_new_type, int> );
static_assert( std::is_trivially_destructible_v<trivial_new_type> );
static_assert( std::is_trivially_copy_constructible_v<trivial_new_type> );
static_assert( std::is_trivially_copy_assignable_v<trivial_new_type> );
static_assert( std::is_trivially_move_constructible_v<trivial_new_type> );
static_assert( std::is_trivially_move_assignable_v<trivial_new_type> );

static_assert( std::is_nothrow_default_constructible_v<trivial_new_type> );
static_assert( std::is_nothrow_constructible_v<trivial_new_type, int> );
static_assert( std::is_nothrow_destructible_v<trivial_new_type> );
static_assert( std::is_nothrow_copy_constructible_v<trivial_new_type> );
static_assert( std::is_nothrow_copy_assignable_v<trivial_new_type> );
static_assert( std::is_nothrow_move_constructible_v<trivial_new_type> );
static_assert( std::is_nothrow_move_assignable_v<trivial_new_type> );

static_assert( shared_static_asserts<trivial_new_type>() );

using complex_new_type = mclo::new_type<std::string, struct complex_new_type_tag>;

static_assert( !std::is_trivially_default_constructible_v<complex_new_type> );
static_assert( !std::is_trivially_constructible_v<complex_new_type, std::string> );
static_assert( !std::is_trivially_destructible_v<complex_new_type> );
static_assert( !std::is_trivially_copy_constructible_v<complex_new_type> );
static_assert( !std::is_trivially_copy_assignable_v<complex_new_type> );
static_assert( !std::is_trivially_move_constructible_v<complex_new_type> );
static_assert( !std::is_trivially_move_assignable_v<complex_new_type> );

static_assert( std::is_nothrow_default_constructible_v<complex_new_type> );
static_assert( std::is_constructible_v<complex_new_type, std::string> );
static_assert( std::is_nothrow_destructible_v<complex_new_type> );
static_assert( std::is_copy_constructible_v<complex_new_type> );
static_assert( std::is_copy_assignable_v<complex_new_type> );
static_assert( std::is_nothrow_move_constructible_v<complex_new_type> );
static_assert( std::is_nothrow_move_assignable_v<complex_new_type> );

static_assert( shared_static_asserts<complex_new_type>() );

static constexpr trivial_new_type constant_trivial_new_type{ 42 };
static_assert( constant_trivial_new_type == 42 );

using test_type_list = mclo::meta::type_list<trivial_new_type, complex_new_type>;

template <typename T>
T test_value() = delete;

template <>
trivial_new_type test_value()
{
	return trivial_new_type{ 42 };
}

template <>
complex_new_type test_value()
{
	return complex_new_type{ "hello" };
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_default_construct_is_default_value", "[new_type]", test_type_list )
{
	const TestType object{};

	CHECK( typename TestType::value_type{} == object.value );
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_lvalue_underlying_construct_is_same_value", "[new_type]", test_type_list )
{
	const auto actual = test_value<TestType>();
	const TestType object{ actual };

	CHECK( actual == object );
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_rvalue_underlying_construct_is_same_value", "[new_type]", test_type_list )
{
	const TestType object{ test_value<TestType>() };

	CHECK( test_value<TestType>() == object );
}

TEST_CASE( "new_type_class_in_place_construct_is_same_value" )
{
	const complex_new_type object( std::in_place, 5, 'a' );

	CHECK( object.value == std::string{ "aaaaa" } );
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_swap_is_swapped", "[new_type]", test_type_list )
{
	TestType lhs( test_value<TestType>() );
	TestType rhs{};

	using std::swap;
	swap( lhs, rhs );

	CHECK( TestType{} == lhs );
	CHECK( test_value<TestType>() == rhs );
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_with_test_value_hash_same_as_underlying", "[new_type]", test_type_list )
{
	const auto actual = test_value<TestType>();
	const TestType object( actual );

	const std::size_t actual_hash = std::hash<typename TestType::value_type>{}( actual );
	const std::size_t object_hash = std::hash<TestType>{}( object );

	CHECK( actual_hash == object_hash );
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_with_value_equality_comparison_same_as_underlying",
						 "[new_type]",
						 test_type_list )
{
	const auto actual = test_value<TestType>();
	const TestType object{ actual };

	const bool actual_result = actual == actual;
	const bool object_result = object == object;

	CHECK( actual_result );
	CHECK( object_result );
}

TEMPLATE_LIST_TEST_CASE( "new_type_class_with_value_spaceship_comparison_same_as_underlying",
						 "[new_type]",
						 test_type_list )
{
	const auto actual = test_value<TestType>();
	const TestType object{ actual };

	const auto actual_result = actual <=> actual;
	const auto object_result = object <=> object;

	const bool same_result = actual_result == object_result;
	CHECK( same_result );
}
