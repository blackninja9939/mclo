#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "assert_macros.hpp"
#include "consteval_check.hpp"

#include "mclo/hash/hash.hpp"
#include "mclo/numeric/fixed_point.hpp"
#include "mclo/numeric/fixed_point_io.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <sstream>

using namespace Catch::Matchers;

namespace
{
	// Binary, 16 fractional bits, scale 65536
	using binary = mclo::fixed_point<std::int32_t, 16>;
	// Unsigned binary, same scale
	using ubinary = mclo::fixed_point<std::uint32_t, 16>;
	// Decimal, 4 fractional digits, scale 10000
	using decimal = mclo::fixed_point<std::int32_t, 4, mclo::fixed_point_base::decimal>;

	constexpr std::int32_t binary_scale = 65536;
	constexpr std::int32_t decimal_scale = 10000;
}

TEST_CASE( "fixed_point default construction is zero", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary().underlying() == 0 );
	CONSTEVAL_CHECK( decimal().underlying() == 0 );
	CONSTEVAL_CHECK( static_cast<double>( binary() ) == 0.0 );
}

TEST_CASE( "fixed_point construction from whole integer", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary( 3 ).underlying() == 3 * binary_scale );
	CONSTEVAL_CHECK( decimal( 3 ).underlying() == 3 * decimal_scale );
	CONSTEVAL_CHECK( binary( -2 ).underlying() == -2 * binary_scale );
}

TEST_CASE( "fixed_point construction from underlying", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary( mclo::from_underlying, binary_scale ).underlying() == binary_scale );
	CONSTEVAL_CHECK( static_cast<double>( binary( mclo::from_underlying, binary_scale ) ) == 1.0 );
}

TEST_CASE( "fixed_point construction from floating point truncates toward zero", "[math][fixed_point]" )
{
	// Decimal represents tenths/hundredths exactly
	CONSTEVAL_CHECK( decimal( 1.25 ).underlying() == 12500 );
	CONSTEVAL_CHECK( decimal( -1.25 ).underlying() == -12500 );

	// Binary represents halves and quarters exactly
	CONSTEVAL_CHECK( binary( 0.5 ).underlying() == binary_scale / 2 );
	CONSTEVAL_CHECK( binary( 0.25 ).underlying() == binary_scale / 4 );
}

TEST_CASE( "fixed_point converts back to floating point", "[math][fixed_point]" )
{
	CHECK_THAT( static_cast<double>( decimal( 1.25 ) ), WithinRel( 1.25 ) );
	CHECK_THAT( static_cast<double>( binary( 0.75 ) ), WithinRel( 0.75 ) );
}

TEST_CASE( "fixed_point addition and subtraction", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary( 2 ) + binary( 3 ) == binary( 5 ) );
	CONSTEVAL_CHECK( binary( 5 ) - binary( 3 ) == binary( 2 ) );

	// Decimal addition is exact for base 10 fractions
	CONSTEVAL_CHECK( ( decimal( 1.25 ) + decimal( 2.50 ) ).underlying() == 37500 );
	CONSTEVAL_CHECK( ( decimal( 0.30 ) + decimal( 0.30 ) + decimal( 0.30 ) ).underlying() == 9000 );
}

TEST_CASE( "fixed_point multiplication", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary( 2 ) * binary( 3 ) == binary( 6 ) );
	CONSTEVAL_CHECK( ( binary( 1.5 ) * binary( 2 ) ).underlying() == 3 * binary_scale );

	CONSTEVAL_CHECK( ( decimal( 1.5 ) * decimal( 2 ) ).underlying() == 3 * decimal_scale );
	CONSTEVAL_CHECK( ( decimal( 0.5 ) * decimal( 0.5 ) ).underlying() == 2500 );
}

TEST_CASE( "fixed_point division truncates toward zero", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary( 6 ) / binary( 2 ) == binary( 3 ) );
	CONSTEVAL_CHECK( ( binary( 1 ) / binary( 2 ) ).underlying() == binary_scale / 2 );

	CONSTEVAL_CHECK( decimal( 6 ) / decimal( 2 ) == decimal( 3 ) );
	CONSTEVAL_CHECK( ( decimal( 1 ) / decimal( 4 ) ).underlying() == 2500 );
}

TEST_CASE( "fixed_point scalar multiplication and division", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( ( binary( 1.5 ) * 2 ).underlying() == 3 * binary_scale );
	CONSTEVAL_CHECK( ( 2 * binary( 1.5 ) ).underlying() == 3 * binary_scale );
	CONSTEVAL_CHECK( ( binary( 3 ) / 2 ).underlying() == binary( 1.5 ).underlying() );
}

TEST_CASE( "fixed_point unary negation", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( ( -binary( 3 ) ).underlying() == -3 * binary_scale );
	CONSTEVAL_CHECK( -binary( 3 ) == binary( -3 ) );
	CONSTEVAL_CHECK( -decimal( 1.25 ) == decimal( -1.25 ) );
}

TEST_CASE( "fixed_point compound assignment", "[math][fixed_point]" )
{
	binary value( 1 );
	value += binary( 2 );
	CHECK( value == binary( 3 ) );
	value -= binary( 1 );
	CHECK( value == binary( 2 ) );
	value *= binary( 3 );
	CHECK( value == binary( 6 ) );
	value /= binary( 2 );
	CHECK( value == binary( 3 ) );
	value *= 2;
	CHECK( value == binary( 6 ) );
	value /= 2;
	CHECK( value == binary( 3 ) );
}

TEST_CASE( "fixed_point increment and decrement", "[math][fixed_point]" )
{
	binary value( 1 );
	CHECK( ++value == binary( 2 ) );
	CHECK( value == binary( 2 ) );
	CHECK( value++ == binary( 2 ) );
	CHECK( value == binary( 3 ) );
	CHECK( --value == binary( 2 ) );
	CHECK( value == binary( 2 ) );
	CHECK( value-- == binary( 2 ) );
	CHECK( value == binary( 1 ) );

	decimal frac( 1.25 );
	++frac;
	CHECK( frac == decimal( 2.25 ) );
	--frac;
	CHECK( frac == decimal( 1.25 ) );
}

TEST_CASE( "fixed_point comparison", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( binary( 1 ) < binary( 2 ) );
	CONSTEVAL_CHECK( binary( 2 ) > binary( 1 ) );
	CONSTEVAL_CHECK( binary( 2 ) == binary( 2 ) );
	CONSTEVAL_CHECK( binary( 1 ) != binary( 2 ) );
	CONSTEVAL_CHECK( decimal( -1.5 ) < decimal( 1.5 ) );
}

TEST_CASE( "fixed_point overflow wraps like a built in integer", "[math][fixed_point]" )
{
	using limits = std::numeric_limits<binary>;
	const binary wrapped = limits::max() + limits::epsilon();
	CONSTEVAL_CHECK( ( limits::max() + limits::epsilon() ).underlying() == std::numeric_limits<std::int32_t>::min() );
	CHECK( wrapped.underlying() == std::numeric_limits<std::int32_t>::min() );
}

TEST_CASE( "fixed_point unsigned representation", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( ubinary( 2 ) + ubinary( 3 ) == ubinary( 5 ) );
	CONSTEVAL_CHECK( ( ubinary( 1.5 ) * ubinary( 2 ) ).underlying() == 3u * binary_scale );
	CONSTEVAL_CHECK( !std::numeric_limits<ubinary>::is_signed );
}

TEST_CASE( "fixed_point 64 bit instantiations", "[math][fixed_point]" )
{
	// 128 bit intermediary arithmetic, exercised at compile time since the extended integer is constexpr capable
	CONSTEVAL_CHECK( mclo::fixed_b64( 2 ) * mclo::fixed_b64( 3 ) == mclo::fixed_b64( 6 ) );
	CONSTEVAL_CHECK( mclo::fixed_b64( 6 ) / mclo::fixed_b64( 2 ) == mclo::fixed_b64( 3 ) );
	CONSTEVAL_CHECK( ( mclo::fixed_d64( 1.5 ) * mclo::fixed_d64( 2 ) ).underlying() == 3 * 1000000000LL );
	CHECK_THAT( static_cast<double>( mclo::fixed_d64( 1.25 ) ), WithinRel( 1.25 ) );
}

TEST_CASE( "fixed_point numeric_limits", "[math][fixed_point]" )
{
	using binary_limits = std::numeric_limits<binary>;
	STATIC_CHECK( binary_limits::is_specialized );
	STATIC_CHECK( binary_limits::is_signed );
	STATIC_CHECK_FALSE( binary_limits::is_integer );
	STATIC_CHECK_FALSE( binary_limits::is_exact );
	STATIC_CHECK( binary_limits::is_bounded );
	STATIC_CHECK( binary_limits::is_modulo );
	STATIC_CHECK( binary_limits::radix == 2 );
	STATIC_CHECK( binary_limits::digits == std::numeric_limits<std::int32_t>::digits );
	STATIC_CHECK( binary_limits::epsilon().underlying() == 1 );
	STATIC_CHECK( binary_limits::max().underlying() == std::numeric_limits<std::int32_t>::max() );
	STATIC_CHECK( binary_limits::min().underlying() == std::numeric_limits<std::int32_t>::min() );

	STATIC_CHECK( std::numeric_limits<decimal>::radix == 10 );
}

TEST_CASE( "fixed_point division by zero asserts", "[math][fixed_point]" )
{
	CHECK_ASSERTS( binary( 1 ) / binary( 0 ), "Division by zero" );
	CHECK_ASSERTS( binary( 1 ) / 0, "Division by zero" );
}

TEST_CASE( "fixed_point user defined literals", "[math][fixed_point]" )
{
	using namespace mclo::literals;

	CONSTEVAL_CHECK( 1.5_fb16 == mclo::fixed_b16( 1.5 ) );
	CONSTEVAL_CHECK( 3_fb16 == mclo::fixed_b16( 3 ) );
	CONSTEVAL_CHECK( 1.5_fb32 == mclo::fixed_b32( 1.5 ) );
	CONSTEVAL_CHECK( 3_fb32 == mclo::fixed_b32( 3 ) );
	CONSTEVAL_CHECK( 1.25_fd32 == mclo::fixed_d32( 1.25 ) );
	CONSTEVAL_CHECK( 3_fd32 == mclo::fixed_d32( 3 ) );

	CHECK( 1.5_fb64 == mclo::fixed_b64( 1.5 ) );
	CHECK( 3_fb64 == mclo::fixed_b64( 3 ) );
	CHECK( 1.25_fd64 == mclo::fixed_d64( 1.25 ) );
	CHECK( 3_fd64 == mclo::fixed_d64( 3 ) );
}

TEST_CASE( "fixed_point abs", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( abs( binary( -2 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( abs( binary( 2 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( abs( binary( 1.5 ) ) == binary( 1.5 ) );
	CONSTEVAL_CHECK( abs( binary( -1.5 ) ) == binary( 1.5 ) );
}

TEST_CASE( "fixed_point floor", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( floor( binary( 1.5 ) ) == binary( 1 ) );
	CONSTEVAL_CHECK( floor( binary( -1.5 ) ) == binary( -2 ) );
	CONSTEVAL_CHECK( floor( binary( 2 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( floor( ubinary( 1.5 ) ) == ubinary( 1 ) );
}

TEST_CASE( "fixed_point ceil", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( ceil( binary( 1.5 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( ceil( binary( -1.5 ) ) == binary( -1 ) );
	CONSTEVAL_CHECK( ceil( binary( 2 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( ceil( ubinary( 1.5 ) ) == ubinary( 2 ) );
}

TEST_CASE( "fixed_point trunc", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( trunc( binary( 1.5 ) ) == binary( 1 ) );
	CONSTEVAL_CHECK( trunc( binary( -1.5 ) ) == binary( -1 ) );
	CONSTEVAL_CHECK( trunc( binary( 2 ) ) == binary( 2 ) );
}

TEST_CASE( "fixed_point round", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( round( binary( 1.5 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( round( binary( 1.25 ) ) == binary( 1 ) );
	CONSTEVAL_CHECK( round( binary( -1.5 ) ) == binary( -2 ) );
	CONSTEVAL_CHECK( round( binary( -1.25 ) ) == binary( -1 ) );
}

TEST_CASE( "fixed_point frac", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( frac( binary( 1.5 ) ) == binary( 0.5 ) );
	CONSTEVAL_CHECK( frac( binary( -1.5 ) ) == binary( -0.5 ) );
	CONSTEVAL_CHECK( frac( binary( 2 ) ) == binary( 0 ) );
}

TEST_CASE( "fixed_point lerp", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( lerp( binary( 2 ), binary( 4 ), binary( 0 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( lerp( binary( 2 ), binary( 4 ), binary( 1 ) ) == binary( 4 ) );
	CONSTEVAL_CHECK( lerp( binary( 2 ), binary( 4 ), binary( 0.5 ) ) == binary( 3 ) );
}

TEST_CASE( "fixed_point clamp", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( clamp( binary( 5 ), binary( 0 ), binary( 3 ) ) == binary( 3 ) );
	CONSTEVAL_CHECK( clamp( binary( -1 ), binary( 0 ), binary( 3 ) ) == binary( 0 ) );
	CONSTEVAL_CHECK( clamp( binary( 2 ), binary( 0 ), binary( 3 ) ) == binary( 2 ) );
	CHECK_ASSERTS( clamp( binary( 2 ), binary( 3 ), binary( 0 ) ), "clamp: low must not be greater than high" );
}

TEST_CASE( "fixed_point std min max clamp work via spaceship", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( std::min( binary( 2 ), binary( 3 ) ) == binary( 2 ) );
	CONSTEVAL_CHECK( std::max( binary( 2 ), binary( 3 ) ) == binary( 3 ) );
	CONSTEVAL_CHECK( std::clamp( binary( 5 ), binary( 0 ), binary( 3 ) ) == binary( 3 ) );
}

TEST_CASE( "fixed_point converting construction preserves value when widening", "[math][fixed_point]" )
{
	using narrow = mclo::fixed_point<std::int32_t, 8>;
	using wide = mclo::fixed_point<std::int64_t, 16>;
	CONSTEVAL_CHECK( wide( narrow( 1.5 ) ) == wide( 1.5 ) );
	CONSTEVAL_CHECK( narrow( wide( 1.5 ) ) == narrow( 1.5 ) );
}

TEST_CASE( "fixed_point converting construction between bases", "[math][fixed_point]" )
{
	CONSTEVAL_CHECK( decimal( binary( 2 ) ) == decimal( 2 ) );
	CONSTEVAL_CHECK( binary( decimal( 2 ) ) == binary( 2 ) );
	// 0.5 is exact in both representations
	CONSTEVAL_CHECK( decimal( binary( 0.5 ) ) == decimal( 0.5 ) );
}

TEST_CASE( "fixed_point converting construction truncates lost precision toward zero", "[math][fixed_point]" )
{
	using coarse = mclo::fixed_point<std::int32_t, 1, mclo::fixed_point_base::decimal>;
	// 0.45 in decimal<4> rescaled to decimal<1> keeps only 0.4
	CONSTEVAL_CHECK( coarse( decimal( 0.45 ) ) == coarse( 0.4 ) );
}

TEST_CASE( "fixed_point hashing", "[math][fixed_point]" )
{
	const std::size_t a = mclo::hash_object( binary( 1.5 ) );
	const std::size_t b = mclo::hash_object( binary( 1.5 ) );
	const std::size_t c = mclo::hash_object( binary( 2.5 ) );
	CHECK( a == b );
	CHECK( a != c );
	CHECK( std::hash<binary>{}( binary( 1.5 ) ) == a );
}

TEST_CASE( "fixed_point stream output is exact", "[math][fixed_point]" )
{
	std::ostringstream os;
	os << binary( 1.5 );
	CHECK( os.str() == "1.5" );

	std::ostringstream negative;
	negative << binary( -2.5 );
	CHECK( negative.str() == "-2.5" );

	std::ostringstream whole;
	whole << decimal( 3 );
	CHECK( whole.str() == "3" );
}

TEST_CASE( "fixed_point stream output beats a double round trip", "[math][fixed_point]" )
{
	// 9223372036.854775807 needs 19 significant digits, more than a double can hold exactly
	std::ostringstream os;
	os << mclo::fixed_d64( mclo::from_underlying, 9223372036854775807LL );
	CHECK( os.str() == "9223372036.854775807" );

	// 2^-32 expands to exactly 32 decimal digits, reproduced exactly from the binary representation
	std::ostringstream binary_exact;
	binary_exact << mclo::fixed_b64( mclo::from_underlying, 1 );
	CHECK( binary_exact.str() == "0.00000000023283064365386962890625" );
}

TEST_CASE( "fixed_point stream input parses exactly", "[math][fixed_point]" )
{
	std::istringstream is( "2.25" );
	binary value;
	is >> value;
	CHECK( value == binary( 2.25 ) );

	std::istringstream negative( "-3.5" );
	binary neg;
	negative >> neg;
	CHECK( neg == binary( -3.5 ) );
}

TEST_CASE( "fixed_point stream input rejects malformed text", "[math][fixed_point]" )
{
	std::istringstream is( "abc" );
	binary value;
	is >> value;
	CHECK( is.fail() );
}

TEST_CASE( "fixed_point stream round trip", "[math][fixed_point]" )
{
	std::stringstream stream;
	stream << decimal( 3.75 );
	decimal value;
	stream >> value;
	CHECK( value == decimal( 3.75 ) );
}

TEST_CASE( "fixed_point format default is exact and trimmed", "[math][fixed_point]" )
{
	CHECK( std::format( "{}", binary( 1.5 ) ) == "1.5" );
	CHECK( std::format( "{}", decimal( 3 ) ) == "3" );
	CHECK( std::format( "{}", mclo::fixed_d64( mclo::from_underlying, 9223372036854775807LL ) ) ==
		   "9223372036.854775807" );
}

TEST_CASE( "fixed_point format with precision rounds half away from zero", "[math][fixed_point]" )
{
	CHECK( std::format( "{:.2}", binary( 1.5 ) ) == "1.50" );
	CHECK( std::format( "{:.0}", binary( 1.5 ) ) == "2" );
	// 1.0050 exactly, rounded to two decimals rounds the half up
	CHECK( std::format( "{:.2}", decimal( mclo::from_underlying, 10050 ) ) == "1.01" );
}
