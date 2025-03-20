#include <catch2/catch_template_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "consteval_check.hpp"

#include "mclo/numeric/normalized_float.hpp"

#include "mclo/meta/type_list.hpp"

using namespace Catch::Matchers;

namespace
{
	using test_types = mclo::meta::type_list<mclo::normalized_float8, mclo::normalized_float16>;

	template <typename T>
	auto NomralizedWithinRel( const float value )
	{
		return WithinRel( value, static_cast<float>( std::numeric_limits<T>::epsilon() ) * 10 );
	}
}

TEMPLATE_LIST_TEST_CASE( "Default Construction", "[math][normalized_float]", test_types )
{
	const TestType nf_default;

	CHECK( static_cast<float>( nf_default ) == 0.0f );
}

TEMPLATE_LIST_TEST_CASE( "Construction from Float", "[math][normalized_float]", test_types )
{
	const TestType nf_from_float( 0.5f );

	CHECK_THAT( static_cast<float>( nf_from_float ), NomralizedWithinRel<TestType>( 0.5f ) );
}

TEMPLATE_LIST_TEST_CASE( "Construction from Underlying Type", "[math][normalized_float]", test_types )
{
	const TestType nf_from_underlying( mclo::from_underlying,
									   std::numeric_limits<typename TestType::underlying_type>::max() );

	CHECK_THAT( static_cast<float>( nf_from_underlying ), WithinRel( 1.0f ) );
}

TEMPLATE_LIST_TEST_CASE( "Assignment from Float", "[math][normalized_float]", test_types )
{
	const TestType nf_default = 0.1f;
	TestType nf_assigned = nf_default;

	nf_assigned = 0.25f;

	CHECK_THAT( static_cast<float>( nf_default ), NomralizedWithinRel<TestType>( 0.1f ) );
	CHECK_THAT( static_cast<float>( nf_assigned ), NomralizedWithinRel<TestType>( 0.25f ) );
}

TEMPLATE_LIST_TEST_CASE( "Arithmetic Operations", "[math][normalized_float]", test_types )
{
	const TestType a( 0.3f );
	const TestType b( 0.4f );
	const TestType c( 0.7f );
	const TestType d( 1.0f );
	using T = typename TestType::underlying_type;

	CHECK( a + b == c );
	CHECK_THAT( d - a, NomralizedWithinRel<TestType>( 0.7f ) );
	CHECK_THAT( a * b, NomralizedWithinRel<TestType>( 0.12f ) );
	CHECK_THAT( a / c, NomralizedWithinRel<TestType>( 0.4285f ) );
	CHECK_THAT( a * T( 2 ), NomralizedWithinRel<TestType>( 0.6f ) );
	CHECK_THAT( c / T( 3 ), NomralizedWithinRel<TestType>( 0.23333f ) );
}

TEMPLATE_LIST_TEST_CASE( "Saturation Behavior", "[math][normalized_float]", test_types )
{
	const TestType min_value( 0.0f );
	const TestType max_value( 1.0f );

	CHECK_THAT( min_value - max_value, NomralizedWithinRel<TestType>( 0.0f ) ); // No underflow
	CHECK_THAT( max_value + max_value, NomralizedWithinRel<TestType>( 1.0f ) ); // No overflow
}

TEMPLATE_LIST_TEST_CASE( "Comparison Operators", "[math][normalized_float]", test_types )
{
	const TestType a( 0.2f );
	const TestType b( 0.5f );
	const TestType c( 0.5f );

	CHECK( ( a <=> b ) == std::strong_ordering::less );
	CHECK( ( b <=> a ) == std::strong_ordering::greater );
	CHECK( ( b <=> c ) == std::strong_ordering::equal );
}

TEMPLATE_LIST_TEST_CASE( "Precision with Larger Integral Types", "[math][normalized_float]", test_types )
{
	const TestType precise_value( 0.123456789f );

	CHECK_THAT( static_cast<float>( precise_value ), NomralizedWithinRel<TestType>( 0.123456789f ) );
}
