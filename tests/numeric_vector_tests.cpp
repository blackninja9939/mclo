#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/numeric/vector.hpp"

#include "mclo/meta/type_list.hpp"

namespace
{
	using test_types = mclo::meta::type_list<mclo::vec2i,
											 mclo::vec3i,
											 mclo::vec4i,
											 mclo::vec2f,
											 mclo::vec3f,
											 mclo::vec4f,
											 mclo::vec2d,
											 mclo::vec3d,
											 mclo::vec4d>;
}

TEMPLATE_LIST_TEST_CASE( "Vector shared constructors", "[vector][math]", test_types )
{
	static_assert( mclo::vec_type<TestType>, "Should meet vec_type concept" );

	using value_type = typename TestType::value_type;

	static_assert( std::is_trivially_copyable_v<TestType> );
	static_assert( std::is_trivially_destructible_v<TestType> );
	static_assert( std::convertible_to<TestType, mclo::span<value_type, TestType::size()>> );
	static_assert( std::convertible_to<const TestType, mclo::span<const value_type, TestType::size()>> );

	SECTION( "Default constructor" )
	{
		TestType vec;
		for ( auto i = 0; i < vec.size(); ++i )
		{
			CHECK( vec[ i ] == 0 );
		}
	}

	SECTION( "Value constructor" )
	{
		TestType vec{ value_type( 1 ) };
		for ( auto i = 0; i < vec.size(); ++i )
		{
			CHECK( vec[ i ] == 1 );
		}
	}

	SECTION( "Copy constructor" )
	{
		TestType vec{ value_type( 1 ) };
		TestType copy( vec );
		for ( auto i = 0; i < vec.size(); ++i )
		{
			CHECK( copy[ i ] == 1 );
		}
	}

	SECTION( "Move constructor" )
	{
		TestType vec{ value_type( 2 ) };
		TestType move( std::move( vec ) );
		for ( auto i = 0; i < move.size(); ++i )
		{
			CHECK( move[ i ] == 2 );
		}
	}

	SECTION( "Span constructor" )
	{
		const auto arr = mclo::broadcast_array<TestType::size()>( value_type( 8 ) );
		TestType vec{ arr };
		for ( auto i = 0; i < vec.size(); ++i )
		{
			CHECK( vec[ i ] == arr[ i ] );
		}
	}
}

TEST_CASE( "Vector from values", "[vector][math]" )
{
	const mclo::vec2i vec2i( 1, 2 );
	CHECK( vec2i[ 0 ] == 1 );
	CHECK( vec2i[ 1 ] == 2 );
	CHECK( vec2i.x() == 1 );
	CHECK( vec2i.y() == 2 );

	const mclo::vec3i vec3i( 1, 2, 3 );
	CHECK( vec3i[ 0 ] == 1 );
	CHECK( vec3i[ 1 ] == 2 );
	CHECK( vec3i[ 2 ] == 3 );
	CHECK( vec3i.x() == 1 );
	CHECK( vec3i.y() == 2 );
	CHECK( vec3i.z() == 3 );

	const mclo::vec4i vec4i( 1, 2, 3, 4 );
	CHECK( vec4i[ 0 ] == 1 );
	CHECK( vec4i[ 1 ] == 2 );
	CHECK( vec4i[ 2 ] == 3 );
	CHECK( vec4i[ 3 ] == 4 );
	CHECK( vec4i.x() == 1 );
	CHECK( vec4i.y() == 2 );
	CHECK( vec4i.z() == 3 );
	CHECK( vec4i.w() == 4 );
}

TEST_CASE( "Vector writeable", "[vector][math]" )
{
	mclo::vec2i vec2i( 1, 2 );
	vec2i[ 0 ] = 3;
	vec2i[ 1 ] = 4;
	CHECK( vec2i[ 0 ] == 3 );
	CHECK( vec2i[ 1 ] == 4 );
	CHECK( vec2i.x() == 3 );
	CHECK( vec2i.y() == 4 );

	mclo::vec3i vec3i( 1, 2, 3 );
	vec3i[ 0 ] = 4;
	vec3i[ 1 ] = 5;
	vec3i[ 2 ] = 6;
	CHECK( vec3i[ 0 ] == 4 );
	CHECK( vec3i[ 1 ] == 5 );
	CHECK( vec3i[ 2 ] == 6 );
	CHECK( vec3i.x() == 4 );
	CHECK( vec3i.y() == 5 );
	CHECK( vec3i.z() == 6 );

	mclo::vec4i vec4i( 1, 2, 3, 4 );
	vec4i[ 0 ] = 5;
	vec4i[ 1 ] = 6;
	vec4i[ 2 ] = 7;
	vec4i[ 3 ] = 8;
	CHECK( vec4i[ 0 ] == 5 );
	CHECK( vec4i[ 1 ] == 6 );
	CHECK( vec4i[ 2 ] == 7 );
	CHECK( vec4i[ 3 ] == 8 );
	CHECK( vec4i.x() == 5 );
	CHECK( vec4i.y() == 6 );
	CHECK( vec4i.z() == 7 );
	CHECK( vec4i.w() == 8 );
}

TEST_CASE( "Vector Addition", "[vector]" )
{
	mclo::vec2i v1( 1, 2 );
	mclo::vec2i v2( 3, 4 );
	mclo::vec2i expected( 4, 6 );

	SECTION( "Operator+" )
	{
		mclo::vec2i result = v1 + v2;
		REQUIRE( result == expected );
	}

	SECTION( "Operator+=" )
	{
		v1 += v2;
		REQUIRE( v1 == expected );
	}
}

TEST_CASE( "Vector Subtraction", "[vector]" )
{
	mclo::vec2i v1( 5, 6 );
	mclo::vec2i v2( 3, 4 );
	mclo::vec2i expected( 2, 2 );

	SECTION( "Operator-" )
	{
		mclo::vec2i result = v1 - v2;
		REQUIRE( result == expected );
	}

	SECTION( "Operator-=" )
	{
		v1 -= v2;
		REQUIRE( v1 == expected );
	}
}

TEST_CASE( "Vector Multiplication", "[vector]" )
{
	mclo::vec2i v1( 2, 3 );
	mclo::vec2i v2( 4, 5 );
	mclo::vec2i expected( 8, 15 );

	SECTION( "Operator*" )
	{
		mclo::vec2i result = v1 * v2;
		REQUIRE( result == expected );
	}

	SECTION( "Operator*=" )
	{
		v1 *= v2;
		REQUIRE( v1 == expected );
	}
}

TEST_CASE( "Vector Division", "[vector]" )
{
	mclo::vec2i v1( 10, 15 );
	mclo::vec2i v2( 2, 3 );
	mclo::vec2i expected( 5, 5 );

	SECTION( "Operator/" )
	{
		mclo::vec2i result = v1 / v2;
		REQUIRE( result == expected );
	}

	SECTION( "Operator/=" )
	{
		v1 /= v2;
		REQUIRE( v1 == expected );
	}
}

TEST_CASE( "Vector Dot Product", "[vector]" )
{
	mclo::vec2i v1( 1, 2 );
	mclo::vec2i v2( 3, 4 );
	const auto expected = 11;

	const auto result = v1.dot( v2 );
	REQUIRE( result == expected );
}

TEST_CASE( "Vector Cross Product", "[vector]" )
{
	mclo::vec3i v1( 1, 2, 3 );
	mclo::vec3i v2( 4, 5, 6 );
	mclo::vec3i expected( -3, 6, -3 );

	const auto result = v1.cross( v2 );
	REQUIRE( result == expected );
}

TEST_CASE( "Vector Norm", "[vector]" )
{
	mclo::vec2i v1( 3, 4 );
	const auto expected = 5;

	const auto result = v1.norm();
	REQUIRE( result == expected );
}

TEST_CASE( "Vector Normalized", "[vector]" )
{
	mclo::vec2f v1( 3, 4.0f );
	const mclo::vec2f expected( 0.6f, 0.8f );

	const mclo::vec2f result = v1.normalized();
	CHECK( result == expected );
}
