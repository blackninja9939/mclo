#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "assert_macros.hpp"

#include "mclo/memory/pointer_variant.hpp"

#include "mclo/utility/overloaded.hpp"

namespace
{
	using test_variant = mclo::pointer_variant<int, bool, float, int>;
	static_assert( test_variant::size == 4 );
	static_assert( std::is_same_v<test_variant::alternative_t<0>, int> );
	static_assert( std::is_same_v<test_variant::alternative_t<1>, bool> );
	static_assert( std::is_same_v<test_variant::alternative_t<2>, float> );
	static_assert( std::is_same_v<test_variant::alternative_t<3>, int> );
}

TEST_CASE( "default pointer_variant, is first variant nullptr", "[pointer_variant]" )
{
	test_variant var;
	CHECK( var.index() == 0 );
	CHECK_FALSE( var.get_raw() );
	CHECK_FALSE( var.get<0>() );
	CHECK_ASSERTS( var.get<3>(), "Variant does not currently hold altnerative" );
}

TEST_CASE( "pointer_variant, construct with int pointer, is first filled variant", "[pointer_variant]" )
{
	int i = 0;
	test_variant var( &i );
	CHECK( var.index() == 0 );
	CHECK( var.get_raw() == &i );
	CHECK( var.get<0>() == &i );
	CHECK_ASSERTS( var.get<3>(), "Variant does not currently hold altnerative" );
}

TEST_CASE( "pointer_variant, construct with index and int pointer, is last filled variant", "[pointer_variant]" )
{
	int i = 0;
	test_variant var( std::in_place_index<3>, &i );
	CHECK( var.index() == 3 );
	CHECK( var.get_raw() == &i );
	CHECK( var.get<3>() == &i );
	CHECK_ASSERTS( var.get<0>(), "Variant does not currently hold altnerative" );
}

TEST_CASE( "pointer_variant, construct with other type pointer, is correctfilled variant", "[pointer_variant]" )
{
	bool i = 0;
	test_variant var( &i );
	CHECK( var.holds_alternative<bool>() );
	CHECK( var.index() == 1 );
	CHECK( var.get_raw() == &i );
	CHECK( var.get<bool>() == &i );
	CHECK( var.get<1>() == &i );
	CHECK_ASSERTS( var.get<0>(), "Variant does not currently hold altnerative" );
}

TEST_CASE( "active pointer_variant, emplace, is new value", "[pointer_variant]" )
{
	bool i = 0;
	float j = 0;
	test_variant var( &i );

	var.emplace( &j );

	CHECK( var.holds_alternative<float>() );
	CHECK( var.index() == 2 );
	CHECK( var.get_raw() == &j );
	CHECK( var.get<float>() == &j );
	CHECK( var.get<2>() == &j );
	CHECK_ASSERTS( var.get<bool>(), "Variant does not currently hold altnerative" );
}

TEST_CASE( "active pointer_variant, assign, is new value", "[pointer_variant]" )
{
	bool i = 0;
	float j = 0;
	test_variant var( &i );

	var = &j;

	CHECK( var.holds_alternative<float>() );
	CHECK( var.index() == 2 );
	CHECK( var.get_raw() == &j );
	CHECK( var.get<float>() == &j );
	CHECK( var.get<2>() == &j );
	CHECK_ASSERTS( var.get<bool>(), "Variant does not currently hold altnerative" );
}

TEST_CASE( "active pointer_variant, write through pointer, modifies original", "[pointer_variant]" )
{
	bool i = false;
	test_variant var( &i );

	*var.get<bool>() = true;

	CHECK( i );
}

TEST_CASE( "const active pointer_variant, read through pointer, reads original modifications", "[pointer_variant]" )
{
	bool i = false;
	const test_variant var( &i );
	i = true;

	CHECK( *var.get<bool>() );
}

TEST_CASE( "active pointer_variant, visit, calls correct overload", "[pointer_variant]" )
{
	float i = 0;
	test_variant var( &i );

	var.visit( mclo::overloaded{ []( float* ptr ) { *ptr = 42.42f; }, []( auto* ) { FAIL( "Should not reach" ); } } );

	CHECK( i == 42.42f );
}

TEST_CASE( "active pointer_variant, visit const, calls correct overload", "[pointer_variant]" )
{
	float i = 42.42f;
	const test_variant var( &i );

	var.visit( mclo::overloaded{ []( const float* ptr ) { CHECK( *ptr == 42.42f ); },
								 []( const auto* ) { FAIL( "Should not reach" ); } } );
}
