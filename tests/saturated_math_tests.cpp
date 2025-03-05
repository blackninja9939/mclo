#include <catch2/catch_template_test_macros.hpp>

#include "assert_macros.hpp"
#include "consteval_check.hpp"

#include "mclo/meta/type_aliases.hpp"

#include "mclo/numeric/saturated_math.hpp"

TEMPLATE_LIST_TEST_CASE( "add_sat performs saturated addition", "[math][saturated_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal addition
	CONSTEVAL_CHECK( mclo::add_sat( T( 1 ), T( 2 ) ) == T( 3 ) );
	CONSTEVAL_CHECK( mclo::add_sat( T( 10 ), T( 20 ) ) == T( 30 ) );

	// Saturation at maximum
	CONSTEVAL_CHECK( mclo::add_sat( max, T( 1 ) ) == max );
	CONSTEVAL_CHECK( mclo::add_sat( max, max ) == max );

	// Saturation at minimum (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::add_sat( min, T( -1 ) ) == min );
		CONSTEVAL_CHECK( mclo::add_sat( min, min ) == min );
	}

	// Zero addition cases
	CONSTEVAL_CHECK( mclo::add_sat( T( 0 ), T( 0 ) ) == T( 0 ) );
	CONSTEVAL_CHECK( mclo::add_sat( T( 0 ), max ) == max );
	CONSTEVAL_CHECK( mclo::add_sat( T( 0 ), min ) == min );
}

TEMPLATE_LIST_TEST_CASE( "sub_sat performs saturated subtraction", "[math][saturated_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal subtraction
	CONSTEVAL_CHECK( mclo::sub_sat( T( 5 ), T( 3 ) ) == T( 2 ) );
	CONSTEVAL_CHECK( mclo::sub_sat( T( 20 ), T( 10 ) ) == T( 10 ) );

	// Saturation at minimum
	CONSTEVAL_CHECK( mclo::sub_sat( min, T( 1 ) ) == min );
	CONSTEVAL_CHECK( mclo::sub_sat( min, max ) == min );

	// Saturation at maximum (only relevant for unsigned types)
	if constexpr ( std::is_unsigned_v<T> )
	{
		CONSTEVAL_CHECK( mclo::sub_sat( T( 0 ), T( 1 ) ) == T( 0 ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "mul_sat performs saturated multiplication", "[math][saturated_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal multiplication
	CONSTEVAL_CHECK( mclo::mul_sat( T( 2 ), T( 3 ) ) == T( 6 ) );
	CONSTEVAL_CHECK( mclo::mul_sat( T( 5 ), T( 4 ) ) == T( 20 ) );

	// Saturation at maximum
	CONSTEVAL_CHECK( mclo::mul_sat( max, T( 2 ) ) == max );
	CONSTEVAL_CHECK( mclo::mul_sat( max, max ) == max );

	// Saturation at minimum (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::mul_sat( min, T( 2 ) ) == min );
		CONSTEVAL_CHECK( mclo::mul_sat( min, min ) == max );
	}
}

TEMPLATE_LIST_TEST_CASE( "div_sat performs saturated division", "[math][saturated_math]", mclo::meta::integers )
{
	using T = TestType;
	constexpr T max = std::numeric_limits<T>::max();
	constexpr T min = std::numeric_limits<T>::min();

	// Normal division
	CONSTEVAL_CHECK( mclo::div_sat( T( 6 ), T( 2 ) ) == T( 3 ) );
	CONSTEVAL_CHECK( mclo::div_sat( T( 20 ), T( 4 ) ) == T( 5 ) );

	// Division by 1
	CONSTEVAL_CHECK( mclo::div_sat( max, T( 1 ) ) == max );
	CONSTEVAL_CHECK( mclo::div_sat( min, T( 1 ) ) == min );

	// Division by -1 (only relevant for signed types)
	if constexpr ( std::is_signed_v<T> )
	{
		CONSTEVAL_CHECK( mclo::div_sat( min, T( -1 ) ) == max );
	}

	// Division by 0 asserts
	CHECK_ASSERTS( mclo::div_sat( T( 6 ), T( 0 ) ), "Division by 0 is undefined behaviour" );
}

TEST_CASE( "Saturate cast tests", "[saturate_cast]" )
{
	SECTION( "Test saturate cast from int to unsigned char" )
	{
		constexpr int int_val = 300;
		constexpr unsigned char saturate_to_uchar = mclo::saturate_cast<unsigned char>( int_val );
		// The value 300 exceeds the range of unsigned char (0-255), so it should saturate to 255
		CONSTEVAL_CHECK( saturate_to_uchar == 255 );
	}

	SECTION( "Test saturate cast from negative int to unsigned char" )
	{
		constexpr int negative_val = -50;
		constexpr unsigned char saturate_negative = mclo::saturate_cast<unsigned char>( negative_val );
		// Negative values should saturate to 0 for unsigned types
		CONSTEVAL_CHECK( saturate_negative == 0 );
	}

	SECTION( "Test saturate cast from int to unsigned char within range" )
	{
		constexpr int within_range_val = 100;
		constexpr unsigned char saturate_within_range = mclo::saturate_cast<unsigned char>( within_range_val );
		// Since 100 is within the unsigned char range (0-255), it should cast directly
		CONSTEVAL_CHECK( saturate_within_range == 100 );
	}

	SECTION( "Test saturate cast from short to int (no saturation)" )
	{
		constexpr short short_val = 32000;
		constexpr int saturate_to_int = mclo::saturate_cast<int>( short_val );
		// Since the value fits within the int range, no saturation is expected
		CONSTEVAL_CHECK( saturate_to_int == 32000 );
	}

	SECTION( "Test saturate cast from int to short (saturates to max short)" )
	{
		constexpr int max_int_val = std::numeric_limits<int>::max();
		constexpr short saturate_to_short_max = mclo::saturate_cast<short>( max_int_val );
		// Since max_int_val exceeds the range of short, it should saturate to the maximum short value
		CONSTEVAL_CHECK( saturate_to_short_max == std::numeric_limits<short>::max() );
	}

	SECTION( "Test saturate cast from negative int to unsigned char (saturates to 0)" )
	{
		constexpr int min_int_val = std::numeric_limits<int>::min();
		constexpr unsigned char saturate_to_uchar_min = mclo::saturate_cast<unsigned char>( min_int_val );
		// Negative values saturate to 0 when casting to unsigned char
		CONSTEVAL_CHECK( saturate_to_uchar_min == 0 );
	}
}
