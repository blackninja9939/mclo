#include <catch2/catch_template_test_macros.hpp>

#include "assert_macros.hpp"
#include "consteval_check.hpp"

#include "mclo/meta/type_aliases.hpp"

#include "mclo/numeric/checked_math.hpp"

TEMPLATE_LIST_TEST_CASE( "add_checked performs checked addition", "[math][checked_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal addition
	CONSTEVAL_CHECK( mclo::add_checked( T( 1 ), T( 2 ) ) == T( 3 ) );
	CONSTEVAL_CHECK( mclo::add_checked( T( 10 ), T( 20 ) ) == T( 30 ) );

	// Empty when overflow at maximum
	CONSTEVAL_CHECK( mclo::add_checked( max, T( 1 ) ) == std::nullopt );
	CONSTEVAL_CHECK( mclo::add_checked( max, max ) == std::nullopt );

	// Empty when overflow at minimum (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::add_checked( min, T( -1 ) ) == std::nullopt );
		CONSTEVAL_CHECK( mclo::add_checked( min, min ) == std::nullopt );
	}

	// Zero addition cases
	CONSTEVAL_CHECK( mclo::add_checked( T( 0 ), T( 0 ) ) == T( 0 ) );
	CONSTEVAL_CHECK( mclo::add_checked( T( 0 ), max ) == max );
	CONSTEVAL_CHECK( mclo::add_checked( T( 0 ), min ) == min );
}

TEMPLATE_LIST_TEST_CASE( "sub_checked performs checked subtraction", "[math][checked_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal subtraction
	CONSTEVAL_CHECK( mclo::sub_checked( T( 5 ), T( 3 ) ) == T( 2 ) );
	CONSTEVAL_CHECK( mclo::sub_checked( T( 20 ), T( 10 ) ) == T( 10 ) );

	// Empty when overflow at minimum
	CONSTEVAL_CHECK( mclo::sub_checked( min, T( 1 ) ) == std::nullopt );
	CONSTEVAL_CHECK( mclo::sub_checked( min, max ) == std::nullopt );

	// Empty when overflow at maximum (only relevant for unsigned types)
	if constexpr ( std::is_unsigned_v<T> )
	{
		CONSTEVAL_CHECK( mclo::sub_checked( T( 0 ), T( 1 ) ) == std::nullopt );
	}
}

TEMPLATE_LIST_TEST_CASE( "mul_checked performs checked multiplication", "[math][checked_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal multiplication
	CONSTEVAL_CHECK( mclo::mul_checked( T( 2 ), T( 3 ) ) == T( 6 ) );
	CONSTEVAL_CHECK( mclo::mul_checked( T( 5 ), T( 4 ) ) == T( 20 ) );

	// Empty when overflow at maximum
	CONSTEVAL_CHECK( mclo::mul_checked( max, T( 2 ) ) == std::nullopt );
	CONSTEVAL_CHECK( mclo::mul_checked( max, max ) == std::nullopt );

	// Empty when overflow at minimum (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::mul_checked( min, T( 2 ) ) == std::nullopt );
		CONSTEVAL_CHECK( mclo::mul_checked( min, min ) == std::nullopt );
	}
}

TEMPLATE_LIST_TEST_CASE( "div_checked performs checked division", "[math][checked_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal division
	CONSTEVAL_CHECK( mclo::div_checked( T( 6 ), T( 2 ) ) == T( 3 ) );
	CONSTEVAL_CHECK( mclo::div_checked( T( 20 ), T( 4 ) ) == T( 5 ) );

	// Division by 1
	CONSTEVAL_CHECK( mclo::div_checked( max, T( 1 ) ) == max );
	CONSTEVAL_CHECK( mclo::div_checked( min, T( 1 ) ) == min );

	// Division by -1 (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::div_checked( min, T( -1 ) ) == std::nullopt );
	}

	// Division by 0 asserts
	CHECK_ASSERTS( mclo::div_checked( T( 6 ), T( 0 ) ), "Division by 0 is undefined behaviour" );
}
