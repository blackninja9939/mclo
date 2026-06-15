#include <catch2/catch_template_test_macros.hpp>

#include "assert_macros.hpp"
#include "consteval_check.hpp"

#include "mclo/meta/type_aliases.hpp"

#include "mclo/numeric/overflowing_math.hpp"
#include "mclo/platform/warnings.hpp"

namespace
{
	template <typename T>
	constexpr std::pair<T, bool> overflow( T x ) noexcept
	{
		return { x, true };
	}
	template <typename T>
	constexpr std::pair<T, bool> no_overflow( T x ) noexcept
	{
		return { x, false };
	}
}

// We are testing overflow
MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( 4307 4310 26450 )

TEMPLATE_LIST_TEST_CASE( "overflowing_add performs overflowing addition",
						 "[math][overflowing_math]",
						 mclo::meta::integers )
{
	using T = TestType;
	using UnsignedT = std::make_unsigned_t<T>;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal addition
	CONSTEVAL_CHECK( mclo::overflowing_add( T( 1 ), T( 2 ) ) == no_overflow( T( 3 ) ) );
	CONSTEVAL_CHECK( mclo::overflowing_add( T( 10 ), T( 20 ) ) == no_overflow( T( 30 ) ) );

	// Overflow at maximum
	CONSTEVAL_CHECK( mclo::overflowing_add( max, T( 1 ) ) == overflow( min ) );
	CONSTEVAL_CHECK( mclo::overflowing_add( max, max ) == overflow( T( UnsignedT( max ) + UnsignedT( max ) ) ) );

	// Overflow at minimum (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::overflowing_add( min, T( -1 ) ) == overflow( max ) );
		CONSTEVAL_CHECK( mclo::overflowing_add( min, min ) == overflow( T( 0 ) ) );
	}

	// Zero addition cases
	CONSTEVAL_CHECK( mclo::overflowing_add( T( 0 ), T( 0 ) ) == no_overflow( T( 0 ) ) );
	CONSTEVAL_CHECK( mclo::overflowing_add( T( 0 ), max ) == no_overflow( max ) );
	CONSTEVAL_CHECK( mclo::overflowing_add( T( 0 ), min ) == no_overflow( min ) );
}

TEMPLATE_LIST_TEST_CASE( "overflowing_sub performs overflowing subtraction",
						 "[math][overflowing_math]",
						 mclo::meta::integers )
{
	using T = TestType;
	using UnsignedT = std::make_unsigned_t<T>;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal subtraction
	CONSTEVAL_CHECK( mclo::overflowing_sub( T( 5 ), T( 3 ) ) == no_overflow( T( 2 ) ) );
	CONSTEVAL_CHECK( mclo::overflowing_sub( T( 20 ), T( 10 ) ) == no_overflow( T( 10 ) ) );

	// Overflow at minimum
	CONSTEVAL_CHECK( mclo::overflowing_sub( min, T( 1 ) ) == overflow( max ) );
	CONSTEVAL_CHECK( mclo::overflowing_sub( min, max ) == overflow( T( UnsignedT( min ) - UnsignedT( max ) ) ) );

	// Overflow at maximum (only relevant for unsigned types)
	if constexpr ( std::is_unsigned_v<T> )
	{
		CONSTEVAL_CHECK( mclo::overflowing_sub( T( 0 ), T( 1 ) ) == overflow( max ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "overflowing_mul performs overflowing multiplication",
						 "[math][overflowing_math]",
						 mclo::meta::integers )
{
	using T = TestType;
	using UnsignedT = std::make_unsigned_t<T>;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal multiplication
	CONSTEVAL_CHECK( mclo::overflowing_mul( T( 2 ), T( 3 ) ) == no_overflow( T( 6 ) ) );
	CONSTEVAL_CHECK( mclo::overflowing_mul( T( 5 ), T( 4 ) ) == no_overflow( T( 20 ) ) );

	// Overflow at maximum
	CONSTEVAL_CHECK( mclo::overflowing_mul( max, T( 2 ) ).second );
	CONSTEVAL_CHECK( mclo::overflowing_mul( max, max ).second );

	// Overflow at minimum (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::overflowing_mul( min, T( 2 ) ).second );
		CONSTEVAL_CHECK( mclo::overflowing_mul( min, min ).second );
	}
}

TEMPLATE_LIST_TEST_CASE( "overflowing_div performs overflowing division",
						 "[math][overflowing_math]",
						 mclo::meta::integers )
{
	using T = TestType;
	using UnsignedT = std::make_unsigned_t<T>;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal division
	CONSTEVAL_CHECK( mclo::overflowing_div( T( 6 ), T( 2 ) ) == no_overflow( T( 3 ) ) );
	CONSTEVAL_CHECK( mclo::overflowing_div( T( 20 ), T( 4 ) ) == no_overflow( T( 5 ) ) );

	// Division by 1
	CONSTEVAL_CHECK( mclo::overflowing_div( max, T( 1 ) ) == no_overflow( max ) );
	CONSTEVAL_CHECK( mclo::overflowing_div( min, T( 1 ) ) == no_overflow( min ) );

	// Division by -1 (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::overflowing_div( min, T( -1 ) ) == overflow( 0 ) );
	}

	// Division by 0 asserts
	CHECK_ASSERTS( mclo::overflowing_div( T( 6 ), T( 0 ) ), "Division by 0 is undefined behaviour" );
}

MCLO_MSVC_POP_WARNINGS
