#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

// Strong type headers
#include "mclo/strong_typedef/arithmetic.hpp"
#include "mclo/strong_typedef/bitwise.hpp"
#include "mclo/strong_typedef/boolean.hpp"
#include "mclo/strong_typedef/decrementable.hpp"
#include "mclo/strong_typedef/default_initialized.hpp"
#include "mclo/strong_typedef/difference.hpp"
#include "mclo/strong_typedef/equality_comparable.hpp"
#include "mclo/strong_typedef/formattable.hpp"
#include "mclo/strong_typedef/hashable.hpp"
#include "mclo/strong_typedef/immovable.hpp"
#include "mclo/strong_typedef/implicitly_convertible.hpp"
#include "mclo/strong_typedef/incrementable.hpp"
#include "mclo/strong_typedef/indexed.hpp"
#include "mclo/strong_typedef/invocable.hpp"
#include "mclo/strong_typedef/move_only.hpp"
#include "mclo/strong_typedef/ordered.hpp"
#include "mclo/strong_typedef/pointer.hpp"
#include "mclo/strong_typedef/range.hpp"
#include "mclo/strong_typedef/regular.hpp"
#include "mclo/strong_typedef/scalable.hpp"
#include "mclo/strong_typedef/semiregular.hpp"
#include "mclo/strong_typedef/streamable.hpp"
#include "mclo/strong_typedef/strong_typedef.hpp"

#include "mclo/hash/fnv1a_hasher.hpp"
#include "mclo/hash/std_types.hpp"
#include "mclo/meta/type_list.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <forward_list>
#include <functional>
#include <limits>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace
{
	template <typename T>
	concept std_hashable = requires( const T& value ) { std::hash<T>{}( value ); };

	template <typename T>
	concept mclo_hashable = mclo::hashable_with<T, mclo::fnv1a_hasher>;
}

// --- Core construction and access ---------------------------------------------------------------

using plain_int = mclo::strong_typedef<int, struct plain_int_tag>;

static_assert( std::is_constructible_v<plain_int, int> );
static_assert( !std::is_default_constructible_v<plain_int> );
static_assert( !std::is_convertible_v<int, plain_int> ); // explicit only
static_assert( !std::is_convertible_v<plain_int, int> ); // no implicit conversion out
static_assert( sizeof( plain_int ) == sizeof( int ) );
static_assert( std::is_same_v<plain_int::value_type, int> );

TEST_CASE( "strong_typedef explicit construction stores the value", "[strong_typedef]" )
{
	const plain_int object{ 42 };
	CHECK( object.value == 42 );
}

TEST_CASE( "strong_typedef value is mutable", "[strong_typedef]" )
{
	plain_int object{ 1 };
	object.value = 2;
	CHECK( object.value == 2 );
}

// --- Zero overhead ------------------------------------------------------------------------------

namespace
{
	// The empty mixin bases must collapse via EBO so the wrapper is layout-compatible with the underlying value.
	template <typename Strong, typename Underlying>
	concept zero_overhead = sizeof( Strong ) == sizeof( Underlying ) && alignof( Strong ) == alignof( Underlying ) &&
							std::is_standard_layout_v<Strong> == std::is_standard_layout_v<Underlying> &&
							std::is_trivially_copyable_v<Strong> == std::is_trivially_copyable_v<Underlying>;
}

using overhead_plain = mclo::strong_typedef<int, struct overhead_plain_tag>;
using overhead_arithmetic = mclo::strong_typedef<int, struct overhead_arithmetic_tag, mclo::arithmetic>;
using overhead_ordered = mclo::strong_typedef<int, struct overhead_ordered_tag, mclo::ordered>;
using overhead_equality = mclo::strong_typedef<int, struct overhead_equality_tag, mclo::equality_comparable>;
using overhead_semiregular = mclo::strong_typedef<int, struct overhead_semiregular_tag, mclo::semiregular>;
using overhead_regular = mclo::strong_typedef<int, struct overhead_regular_tag, mclo::regular>;
using overhead_composed = mclo::strong_typedef<int,
											   struct overhead_composed_tag,
											   mclo::arithmetic,
											   mclo::ordered,
											   mclo::semiregular,
											   mclo::hashable>;

static_assert( zero_overhead<overhead_plain, int> );
static_assert( zero_overhead<overhead_arithmetic, int> );
static_assert( zero_overhead<overhead_ordered, int> );
static_assert( zero_overhead<overhead_equality, int> );
static_assert( zero_overhead<overhead_semiregular, int> );
static_assert( zero_overhead<overhead_regular, int> );
static_assert( zero_overhead<overhead_composed, int> );

// --- Default construction marker ----------------------------------------------------------------

using default_int = mclo::strong_typedef<int, struct default_int_tag, mclo::default_initialized>;

static_assert( std::is_default_constructible_v<default_int> );
static_assert( std::is_trivially_default_constructible_v<default_int> );
static_assert( sizeof( default_int ) == sizeof( int ) );

TEST_CASE( "strong_typedef default_initialized value initialises the underlying", "[strong_typedef]" )
{
	const default_int object{};
	CHECK( object.value == 0 );
}

// --- Copy/move propagation ----------------------------------------------------------------------

using trivial_int = mclo::strong_typedef<int, struct trivial_int_tag, mclo::default_initialized>;
using string_type = mclo::strong_typedef<std::string, struct string_type_tag, mclo::default_initialized>;

static_assert( std::is_trivially_copyable_v<trivial_int> );
static_assert( std::is_trivially_copy_constructible_v<trivial_int> );
static_assert( std::is_trivially_move_constructible_v<trivial_int> );

static_assert( !std::is_trivially_copyable_v<string_type> );
static_assert( std::is_copy_constructible_v<string_type> );
static_assert( std::is_move_constructible_v<string_type> );
static_assert( std::is_nothrow_move_constructible_v<string_type> );

using move_only_string =
	mclo::strong_typedef<std::string, struct move_only_tag, mclo::default_initialized, mclo::move_only>;

static_assert( !std::is_copy_constructible_v<move_only_string> );
static_assert( !std::is_copy_assignable_v<move_only_string> );
static_assert( std::is_move_constructible_v<move_only_string> );
static_assert( std::is_move_assignable_v<move_only_string> );

using immovable_string =
	mclo::strong_typedef<std::string, struct immovable_tag, mclo::default_initialized, mclo::immovable>;

static_assert( !std::is_copy_constructible_v<immovable_string> );
static_assert( !std::is_move_constructible_v<immovable_string> );

// --- Swap ---------------------------------------------------------------------------------------

TEST_CASE( "strong_typedef swap exchanges values", "[strong_typedef]" )
{
	string_type lhs{ std::string{ "hello" } };
	string_type rhs{ std::string{ "world" } };

	using std::swap;
	swap( lhs, rhs );

	CHECK( lhs.value == "world" );
	CHECK( rhs.value == "hello" );
}

// --- Implicit conversion mixin ------------------------------------------------------------------

using convertible_int = mclo::strong_typedef<int, struct convertible_tag, mclo::implicitly_convertible>;

static_assert( std::is_convertible_v<convertible_int, int> );
static_assert( std::is_nothrow_convertible_v<convertible_int, const int&> );

TEST_CASE( "strong_typedef implicitly_convertible converts to the underlying", "[strong_typedef]" )
{
	const convertible_int object{ 7 };
	const int converted = object;
	CHECK( converted == 7 );
}

TEST_CASE( "strong_typedef implicitly_convertible exposes a mutable reference", "[strong_typedef]" )
{
	convertible_int object{ 7 };
	int& ref = object;
	ref = 9;
	CHECK( object.value == 9 );
}

using convertible_to_long = mclo::strong_typedef<int, struct convertible_to_tag, mclo::implicitly_convertible_to<long>>;

static_assert( std::is_convertible_v<convertible_to_long, long> );
static_assert( std::is_nothrow_convertible_v<convertible_to_long, long> );
static_assert( !std::is_convertible_v<convertible_to_long, std::string> );

TEST_CASE( "strong_typedef implicitly_convertible_to converts to a specific type", "[strong_typedef]" )
{
	const convertible_to_long object{ 7 };
	const long converted = object;
	CHECK( converted == 7L );
}

// --- Difference mixin ---------------------------------------------------------------------------

using offset = mclo::strong_typedef<std::ptrdiff_t, struct offset_tag>;
using position = mclo::strong_typedef<std::ptrdiff_t, struct position_tag, mclo::difference<offset>>;

static_assert( noexcept( std::declval<position>() - std::declval<position>() ) );

TEST_CASE( "strong_typedef difference subtracts to a distinct type", "[strong_typedef]" )
{
	const position a{ 10 };
	const position b{ 4 };
	const offset diff = a - b;
	static_assert( std::is_same_v<decltype( a - b ), offset> );
	CHECK( diff.value == 6 );
}

// --- Comparison mixins --------------------------------------------------------------------------

using equatable_int = mclo::strong_typedef<int, struct equatable_tag, mclo::equality_comparable>;
using ordered_int = mclo::strong_typedef<int, struct ordered_tag, mclo::ordered>;

static_assert( std::equality_comparable<equatable_int> );
static_assert( !std::three_way_comparable<equatable_int> );
static_assert( std::three_way_comparable<ordered_int> );

static_assert( noexcept( std::declval<equatable_int>() == std::declval<equatable_int>() ) );
static_assert( noexcept( std::declval<ordered_int>() == std::declval<ordered_int>() ) );
static_assert( noexcept( std::declval<ordered_int>() <=> std::declval<ordered_int>() ) );

TEST_CASE( "strong_typedef equality_comparable compares equality", "[strong_typedef]" )
{
	CHECK( equatable_int{ 1 } == equatable_int{ 1 } );
	CHECK( equatable_int{ 1 } != equatable_int{ 2 } );
}

TEST_CASE( "strong_typedef ordered compares relationally", "[strong_typedef]" )
{
	CHECK( ordered_int{ 1 } < ordered_int{ 2 } );
	CHECK( ordered_int{ 2 } <= ordered_int{ 2 } );
	CHECK( ordered_int{ 3 } > ordered_int{ 2 } );
	CHECK( ordered_int{ 2 } == ordered_int{ 2 } );
	CHECK( ordered_int{ 2 } != ordered_int{ 3 } );
}

namespace
{
	// A legacy value type providing only operator< and operator==, exercising the synth_three_way fallback path.
	struct only_less_than
	{
		int value = 0;
		[[nodiscard]] friend constexpr bool operator<( const only_less_than& lhs, const only_less_than& rhs ) noexcept
		{
			return lhs.value < rhs.value;
		}
		[[nodiscard]] friend constexpr bool operator==( const only_less_than& lhs, const only_less_than& rhs ) noexcept
		{
			return lhs.value == rhs.value;
		}
	};
}

using synth_ordered = mclo::strong_typedef<only_less_than, struct synth_ordered_tag, mclo::ordered>;

static_assert( std::three_way_comparable<synth_ordered> );
static_assert( std::is_same_v<std::compare_three_way_result_t<synth_ordered>, std::weak_ordering> );

TEST_CASE( "strong_typedef ordered synthesises ordering for less-than-only types", "[strong_typedef]" )
{
	const synth_ordered a{ only_less_than{ 1 } };
	const synth_ordered b{ only_less_than{ 2 } };

	CHECK( a < b );
	CHECK( b > a );
	CHECK( a <= b );
	CHECK( a == a );
	CHECK( a != b );
	CHECK( ( a <=> b ) == std::weak_ordering::less );
}

// --- Heterogeneous comparison mixins ------------------------------------------------------------

using equatable_with_int = mclo::strong_typedef<int, struct equatable_with_tag, mclo::equality_comparable_with<int>>;
using ordered_with_int = mclo::strong_typedef<int, struct ordered_with_tag, mclo::ordered_with<int>>;

// The std cross-type comparison concepts additionally require homogeneous comparability and a common reference, which a
// strong type and its underlying do not share, so assert the heterogeneous operators directly instead.
static_assert( requires( const equatable_with_int a, const int b ) {
	{ a == b } -> std::same_as<bool>;
	{ b == a } -> std::same_as<bool>;
	{ a != b } -> std::same_as<bool>;
} );
static_assert( requires( const ordered_with_int a, const int b ) {
	{ a < b } -> std::same_as<bool>;
	{ b < a } -> std::same_as<bool>;
	{ a == b } -> std::same_as<bool>;
	a <=> b;
} );

TEST_CASE( "strong_typedef equality_comparable_with compares against another type", "[strong_typedef]" )
{
	const equatable_with_int object{ 5 };

	CHECK( object == 5 );
	CHECK( 5 == object );
	CHECK( object != 6 );
	CHECK( 6 != object );
}

TEST_CASE( "strong_typedef ordered_with compares against another type", "[strong_typedef]" )
{
	const ordered_with_int object{ 5 };

	CHECK( object < 6 );
	CHECK( 6 > object );
	CHECK( object > 4 );
	CHECK( 4 < object );
	CHECK( object <= 5 );
	CHECK( object >= 5 );
	CHECK( object == 5 );
	CHECK( 5 == object );
	CHECK( object != 6 );
	CHECK( ( object <=> 6 ) == std::strong_ordering::less );
}

// --- Arithmetic mixins (isolated) ---------------------------------------------------------------

using add_int = mclo::strong_typedef<int, struct add_tag, mclo::addable>;
using sub_int = mclo::strong_typedef<int, struct sub_tag, mclo::subtractable>;
using mul_int = mclo::strong_typedef<int, struct mul_tag, mclo::multipliable>;
using div_int = mclo::strong_typedef<int, struct div_tag, mclo::dividable>;
using mod_int = mclo::strong_typedef<int, struct mod_tag, mclo::modulable>;
using neg_int = mclo::strong_typedef<int, struct neg_tag, mclo::negatable>;
using inc_int = mclo::strong_typedef<int, struct inc_tag, mclo::incrementable>;
using dec_int = mclo::strong_typedef<int, struct dec_tag, mclo::decrementable>;

// Each component mixin contributes only its own behaviour and composes nothing else.
static_assert( mclo::has_mixin<add_int, mclo::addable> && !mclo::has_mixin<add_int, mclo::subtractable> );
static_assert( mclo::has_mixin<sub_int, mclo::subtractable> && !mclo::has_mixin<sub_int, mclo::addable> );
static_assert( mclo::has_mixin<mul_int, mclo::multipliable> && !mclo::has_mixin<mul_int, mclo::dividable> );
static_assert( mclo::has_mixin<div_int, mclo::dividable> && !mclo::has_mixin<div_int, mclo::multipliable> );
static_assert( mclo::has_mixin<mod_int, mclo::modulable> && !mclo::has_mixin<mod_int, mclo::addable> );
static_assert( mclo::has_mixin<neg_int, mclo::negatable> && !mclo::has_mixin<neg_int, mclo::subtractable> );
static_assert( mclo::has_mixin<inc_int, mclo::incrementable> && !mclo::has_mixin<inc_int, mclo::decrementable> );
static_assert( mclo::has_mixin<dec_int, mclo::decrementable> && !mclo::has_mixin<dec_int, mclo::incrementable> );

// noexcept propagates from the underlying int operations.
static_assert( noexcept( std::declval<add_int>() + std::declval<add_int>() ) );
static_assert( noexcept( std::declval<add_int&>() += std::declval<add_int>() ) );
static_assert( noexcept( std::declval<sub_int>() - std::declval<sub_int>() ) );
static_assert( noexcept( std::declval<sub_int&>() -= std::declval<sub_int>() ) );
static_assert( noexcept( std::declval<mul_int>() * std::declval<mul_int>() ) );
static_assert( noexcept( std::declval<div_int>() / std::declval<div_int>() ) );
static_assert( noexcept( std::declval<mod_int>() % std::declval<mod_int>() ) );
static_assert( noexcept( -std::declval<neg_int>() ) );
static_assert( noexcept( ++std::declval<inc_int&>() ) );
static_assert( noexcept( std::declval<inc_int&>()++ ) );
static_assert( noexcept( --std::declval<dec_int&>() ) );
static_assert( noexcept( std::declval<dec_int&>()-- ) );

TEST_CASE( "strong_typedef addable adds and compound adds", "[strong_typedef]" )
{
	CHECK( ( add_int{ 12 } + add_int{ 4 } ).value == 16 );
	add_int a{ 10 };
	a += add_int{ 5 };
	CHECK( a.value == 15 );
}

TEST_CASE( "strong_typedef subtractable subtracts and compound subtracts", "[strong_typedef]" )
{
	CHECK( ( sub_int{ 12 } - sub_int{ 4 } ).value == 8 );
	sub_int a{ 12 };
	a -= sub_int{ 3 };
	CHECK( a.value == 9 );
}

TEST_CASE( "strong_typedef multipliable multiplies and compound multiplies", "[strong_typedef]" )
{
	CHECK( ( mul_int{ 12 } * mul_int{ 4 } ).value == 48 );
	mul_int a{ 6 };
	a *= mul_int{ 2 };
	CHECK( a.value == 12 );
}

TEST_CASE( "strong_typedef dividable divides and compound divides", "[strong_typedef]" )
{
	CHECK( ( div_int{ 12 } / div_int{ 4 } ).value == 3 );
	div_int a{ 24 };
	a /= div_int{ 4 };
	CHECK( a.value == 6 );
}

TEST_CASE( "strong_typedef modulable mods and compound mods", "[strong_typedef]" )
{
	CHECK( ( mod_int{ 13 } % mod_int{ 4 } ).value == 1 );
	mod_int a{ 14 };
	a %= mod_int{ 4 };
	CHECK( a.value == 2 );
}

TEST_CASE( "strong_typedef negatable negates", "[strong_typedef]" )
{
	CHECK( ( -neg_int{ 5 } ).value == -5 );
	CHECK( ( -neg_int{ -3 } ).value == 3 );
}

TEST_CASE( "strong_typedef incrementable pre and post increments", "[strong_typedef]" )
{
	inc_int a{ 5 };
	CHECK( ( ++a ).value == 6 );
	CHECK( ( a++ ).value == 6 );
	CHECK( a.value == 7 );
}

TEST_CASE( "strong_typedef decrementable pre and post decrements", "[strong_typedef]" )
{
	dec_int a{ 5 };
	CHECK( ( --a ).value == 4 );
	CHECK( ( a-- ).value == 4 );
	CHECK( a.value == 3 );
}

// --- Arithmetic bundle --------------------------------------------------------------------------

using math_int = mclo::strong_typedef<int, struct math_tag, mclo::arithmetic>;

// The bundle composes every component mixin; validate the composition structurally via has_mixin.
static_assert( mclo::has_mixin<math_int, mclo::addable> );
static_assert( mclo::has_mixin<math_int, mclo::subtractable> );
static_assert( mclo::has_mixin<math_int, mclo::multipliable> );
static_assert( mclo::has_mixin<math_int, mclo::dividable> );
static_assert( mclo::has_mixin<math_int, mclo::modulable> );
static_assert( mclo::has_mixin<math_int, mclo::negatable> );
static_assert( mclo::has_mixin<math_int, mclo::incrementable> );
static_assert( mclo::has_mixin<math_int, mclo::decrementable> );
static_assert( mclo::has_mixin<math_int, mclo::arithmetic> );
static_assert( !mclo::has_mixin<plain_int, mclo::addable> );

TEST_CASE( "strong_typedef arithmetic bundle exposes the full operator set", "[strong_typedef]" )
{
	math_int a{ 12 };
	const math_int b{ 4 };

	CHECK( ( a + b ).value == 16 );
	CHECK( ( a - b ).value == 8 );
	CHECK( ( a * b ).value == 48 );
	CHECK( ( a / b ).value == 3 );
	CHECK( ( a % b ).value == 0 );
	CHECK( ( -a ).value == -12 );

	a += b;
	CHECK( a.value == 16 );
	CHECK( ( ++a ).value == 17 );
	CHECK( ( --a ).value == 16 );
}

namespace
{
	// A value type whose binary operations and comparisons are potentially-throwing (left un-noexcept), used to confirm
	// the mixins propagate noexcept(false) from the underlying type.
	struct throwing_int
	{
		int value = 0;

		throwing_int() = default;
		constexpr throwing_int( int v ) noexcept
			: value( v )
		{
		}

		[[nodiscard]] friend throwing_int operator+( const throwing_int& lhs, const throwing_int& rhs )
		{
			return throwing_int{ lhs.value + rhs.value };
		}
		[[nodiscard]] friend bool operator==( const throwing_int& lhs, const throwing_int& rhs )
		{
			return lhs.value == rhs.value;
		}
		[[nodiscard]] friend std::strong_ordering operator<=>( const throwing_int& lhs, const throwing_int& rhs )
		{
			return lhs.value <=> rhs.value;
		}
	};
}

using throwing_strong = mclo::strong_typedef<throwing_int, struct throwing_tag, mclo::addable, mclo::ordered>;

static_assert( !noexcept( std::declval<throwing_strong>() + std::declval<throwing_strong>() ) );
static_assert( !noexcept( std::declval<throwing_strong>() == std::declval<throwing_strong>() ) );
static_assert( !noexcept( std::declval<throwing_strong>() <=> std::declval<throwing_strong>() ) );

// --- Bitwise and shift mixins -------------------------------------------------------------------

using bits = mclo::strong_typedef<unsigned, struct bits_tag, mclo::bitwise>;

static_assert( noexcept( std::declval<bits>() & std::declval<bits>() ) );
static_assert( noexcept( std::declval<bits>() | std::declval<bits>() ) );
static_assert( noexcept( std::declval<bits>() ^ std::declval<bits>() ) );
static_assert( noexcept( ~std::declval<bits>() ) );

TEST_CASE( "strong_typedef bitwise operations", "[strong_typedef]" )
{
	const bits x{ 0b1100u };
	const bits y{ 0b1010u };

	CHECK( ( x & y ).value == 0b1000u );
	CHECK( ( x | y ).value == 0b1110u );
	CHECK( ( x ^ y ).value == 0b0110u );
	CHECK( ( ~x ).value == ~0b1100u );

	bits z{ 0b1100u };
	z &= y;
	CHECK( z.value == 0b1000u );
	z |= bits{ 0b0001u };
	CHECK( z.value == 0b1001u );
	z ^= bits{ 0b1111u };
	CHECK( z.value == 0b0110u );
}

using shift_int = mclo::strong_typedef<unsigned, struct shift_tag, mclo::bitwise>;

TEST_CASE( "strong_typedef bitwise shift operations", "[strong_typedef]" )
{
	CHECK( ( shift_int{ 1u } << 3 ).value == 8u );
	CHECK( ( shift_int{ 16u } >> 2 ).value == 4u );

	shift_int value{ 1u };
	value <<= 4;
	CHECK( value.value == 16u );
	value >>= 1;
	CHECK( value.value == 8u );
}

// --- Hashable mixin -----------------------------------------------------------------------------

using hashable_int = mclo::strong_typedef<int, struct hashable_tag, mclo::hashable>;

static_assert( std_hashable<hashable_int> );
static_assert( !std_hashable<plain_int> );
static_assert( mclo_hashable<hashable_int> );
static_assert( noexcept( std::hash<hashable_int>{}( std::declval<hashable_int>() ) ) );
static_assert( mclo::has_mixin<hashable_int, mclo::hashable> );

TEST_CASE( "strong_typedef hashable matches the underlying std::hash", "[strong_typedef]" )
{
	CHECK( std::hash<hashable_int>{}( hashable_int{ 13 } ) == std::hash<int>{}( 13 ) );
}

TEST_CASE( "strong_typedef hashable hash_append matches the underlying", "[strong_typedef]" )
{
	mclo::fnv1a_hasher object_hasher;
	hash_append( object_hasher, hashable_int{ 13 } );

	mclo::fnv1a_hasher value_hasher;
	hash_append( value_hasher, 13 );

	CHECK( object_hasher.finish() == value_hasher.finish() );
}

// --- Formattable and streamable mixins ----------------------------------------------------------

using formattable_int = mclo::strong_typedef<int, struct formattable_tag, mclo::formattable>;

static_assert( noexcept( format_as( std::declval<formattable_int>() ) ) );

TEST_CASE( "strong_typedef formattable exposes value via format_as", "[strong_typedef]" )
{
	CHECK( format_as( formattable_int{ 99 } ) == 99 );
}

using ostreamable_int = mclo::strong_typedef<int, struct ostreamable_tag, mclo::ostreamable>;

TEST_CASE( "strong_typedef ostreamable inserts the value into a stream", "[strong_typedef]" )
{
	std::ostringstream stream;
	stream << ostreamable_int{ 42 };
	CHECK( stream.str() == "42" );
}

using istreamable_int = mclo::strong_typedef<int, struct istreamable_tag, mclo::istreamable>;

TEST_CASE( "strong_typedef istreamable extracts the value from a stream", "[strong_typedef]" )
{
	std::istringstream stream{ "42" };
	istreamable_int object{ 0 };
	stream >> object;
	CHECK( object.value == 42 );
}

using iostreamable_int = mclo::strong_typedef<int, struct iostreamable_tag, mclo::iostreamable>;

static_assert( mclo::has_mixin<iostreamable_int, mclo::istreamable> );
static_assert( mclo::has_mixin<iostreamable_int, mclo::ostreamable> );
static_assert( mclo::has_mixin<iostreamable_int, mclo::iostreamable> );

TEST_CASE( "strong_typedef iostreamable round trips through a stream", "[strong_typedef]" )
{
	std::stringstream stream;
	stream << iostreamable_int{ 7 };

	iostreamable_int object{ 0 };
	stream >> object;
	CHECK( object.value == 7 );
}

// --- Preset bundles -----------------------------------------------------------------------------

using number = mclo::
	strong_typedef<int, struct number_tag, mclo::arithmetic, mclo::ordered, mclo::hashable, mclo::default_initialized>;

static_assert( std::three_way_comparable<number> );
static_assert( std_hashable<number> );
static_assert( sizeof( number ) == sizeof( int ) );
static_assert( std::numeric_limits<number>::is_specialized );
static_assert( std::numeric_limits<number>::is_signed == std::numeric_limits<int>::is_signed );
static_assert( std::numeric_limits<number>::digits == std::numeric_limits<int>::digits );

TEST_CASE( "strong_typedef composes arithmetic, ordering and hashing", "[strong_typedef]" )
{
	const number a{ 3 };
	const number b{ 4 };

	CHECK( ( a + b ) == number{ 7 } );
	CHECK( a < b );
	CHECK( std::hash<number>{}( a ) == std::hash<int>{}( 3 ) );
}

TEST_CASE( "strong_typedef arithmetic specialises std::numeric_limits", "[strong_typedef]" )
{
	CHECK( std::numeric_limits<number>::min() == number{ std::numeric_limits<int>::min() } );
	CHECK( std::numeric_limits<number>::max() == number{ std::numeric_limits<int>::max() } );
	CHECK( std::numeric_limits<number>::lowest() == number{ std::numeric_limits<int>::lowest() } );
}

using semiregular_string = mclo::strong_typedef<std::string, struct semiregular_tag, mclo::semiregular>;

static_assert( std::is_default_constructible_v<semiregular_string> );
static_assert( std::is_copy_constructible_v<semiregular_string> );
static_assert( std::is_move_constructible_v<semiregular_string> );
static_assert( !std::equality_comparable<semiregular_string> );

TEST_CASE( "strong_typedef semiregular bundle enables default construction", "[strong_typedef]" )
{
	const semiregular_string object{};
	CHECK( object.value.empty() );
}

using regular_string = mclo::strong_typedef<std::string, struct regular_tag, mclo::regular>;

static_assert( std::is_default_constructible_v<regular_string> );
static_assert( std::equality_comparable<regular_string> );
static_assert( !std_hashable<regular_string> );
static_assert( mclo::has_mixin<regular_string, mclo::semiregular> );
static_assert( mclo::has_mixin<regular_string, mclo::equality_comparable> );
static_assert( mclo::has_mixin<regular_string, mclo::regular> );

TEST_CASE( "strong_typedef regular bundle composes default construction and equality", "[strong_typedef]" )
{
	CHECK( regular_string{} == regular_string{ std::string{} } );
	CHECK( regular_string{ std::string{ "a" } } == regular_string{ std::string{ "a" } } );
	CHECK( regular_string{ std::string{ "a" } } != regular_string{ std::string{ "b" } } );
}

// --- Boolean mixin ------------------------------------------------------------------------------

using boolean_int = mclo::strong_typedef<int, struct boolean_tag, mclo::boolean>;

static_assert( !std::is_convertible_v<boolean_int, bool> ); // explicit only
static_assert( std::is_constructible_v<bool, boolean_int> );
static_assert( noexcept( static_cast<bool>( std::declval<boolean_int>() ) ) );

TEST_CASE( "strong_typedef boolean exposes explicit operator bool", "[strong_typedef]" )
{
	CHECK( static_cast<bool>( boolean_int{ 5 } ) );
	CHECK_FALSE( static_cast<bool>( boolean_int{ 0 } ) );
	const boolean_int object{ 1 };

	if ( object )
	{
		SUCCEED( "contextual conversion works" );
	}
	else
	{
		FAIL( "expected true" );
	}
}

// --- Pointer mixin ------------------------------------------------------------------------------

using int_pointer = mclo::strong_typedef<int*, struct int_pointer_tag, mclo::pointer>;

static_assert( noexcept( std::declval<int_pointer>() == nullptr ) );

TEST_CASE( "strong_typedef pointer exposes dereference, arrow and nullptr comparison", "[strong_typedef]" )
{
	int storage = 42;
	const int_pointer object{ &storage };

	CHECK( *object == 42 );
	CHECK( object.operator->() == &storage );
	CHECK( object != nullptr );

	*object = 7;
	CHECK( storage == 7 );

	const int_pointer empty{ nullptr };
	CHECK( empty == nullptr );
}

// --- Invocable mixin ----------------------------------------------------------------------------

using int_adder = mclo::strong_typedef<std::function<int( int, int )>, struct int_adder_tag, mclo::invocable>;

TEST_CASE( "strong_typedef invocable forwards to the wrapped callable", "[strong_typedef]" )
{
	const int_adder object{ std::function<int( int, int )>{ []( int a, int b ) { return a + b; } } };
	CHECK( object( 2, 3 ) == 5 );
}

namespace
{
	// A callable with a non-const operator() so the non-const invocable overload is exercised distinctly.
	struct mutable_counter
	{
		int count = 0;
		int operator()( int by )
		{
			count += by;
			return count;
		}
	};

	// A callable taking a move-only argument, used to confirm arguments are perfectly forwarded.
	struct sink
	{
		int operator()( std::unique_ptr<int> p ) const
		{
			return *p;
		}
	};
}

using counter_fn = mclo::strong_typedef<mutable_counter, struct counter_fn_tag, mclo::invocable>;
using sink_fn = mclo::strong_typedef<sink, struct sink_fn_tag, mclo::invocable>;

TEST_CASE( "strong_typedef invocable forwards through the non-const overload", "[strong_typedef]" )
{
	counter_fn object{ mutable_counter{} };
	CHECK( object( 3 ) == 3 );
	CHECK( object( 4 ) == 7 );
}

TEST_CASE( "strong_typedef invocable perfectly forwards arguments", "[strong_typedef]" )
{
	const sink_fn object{ sink{} };
	CHECK( object( std::make_unique<int>( 42 ) ) == 42 );
}

// --- Indexed mixin ------------------------------------------------------------------------------

using indexed_array = mclo::strong_typedef<std::array<int, 3>, struct indexed_array_tag, mclo::indexed<std::size_t>>;

TEST_CASE( "strong_typedef indexed exposes a subscript operator", "[strong_typedef]" )
{
	indexed_array object{
		std::array<int, 3>{ 10, 20, 30 }
    };

	CHECK( object[ 0 ] == 10 );
	CHECK( object[ 2 ] == 30 );

	object[ 1 ] = 99;
	CHECK( object.value[ 1 ] == 99 );

	const indexed_array& cref = object;
	CHECK( cref[ 0 ] == 10 );
	CHECK( cref[ 1 ] == 99 );
}

// --- Scalable mixin -----------------------------------------------------------------------------

using meters = mclo::strong_typedef<double, struct meters_tag, mclo::scalable_with<double>>;

static_assert( mclo::has_mixin<meters, mclo::scalable_with<double>> );
static_assert( noexcept( std::declval<meters>() * std::declval<double>() ) );
static_assert( noexcept( std::declval<double>() * std::declval<meters>() ) );
static_assert( noexcept( std::declval<meters>() / std::declval<double>() ) );

// Scaling is only against the scalar; it does not introduce a strong * strong product.
static_assert( !mclo::has_mixin<meters, mclo::multipliable> );

TEST_CASE( "strong_typedef scalable_with scales by a scalar on either side", "[strong_typedef]" )
{
	CHECK( ( meters{ 3.0 } * 2.0 ).value == 6.0 );
	CHECK( ( 2.0 * meters{ 3.0 } ).value == 6.0 );
	CHECK( ( meters{ 6.0 } / 2.0 ).value == 3.0 );

	meters m{ 4.0 };
	m *= 2.0;
	CHECK( m.value == 8.0 );
	m /= 4.0;
	CHECK( m.value == 2.0 );
}

// --- Range mixin --------------------------------------------------------------------------------

using int_vector = mclo::strong_typedef<std::vector<int>, struct int_vector_tag, mclo::range, mclo::semiregular>;
using fixed_ints = mclo::strong_typedef<std::array<int, 3>, struct fixed_ints_tag, mclo::range>;
using int_list = mclo::strong_typedef<std::forward_list<int>, struct int_list_tag, mclo::range>;

static_assert( std::ranges::range<int_vector> );
static_assert( std::ranges::range<const int_vector> );
static_assert( std::ranges::sized_range<int_vector> );

// A non-sized underlying range still models a range, but does not gain size() / empty().
static_assert( std::ranges::range<int_list> );
static_assert( !std::ranges::sized_range<int_list> );

TEST_CASE( "strong_typedef range iterates and dispatches to ranges algorithms", "[strong_typedef]" )
{
	int_vector object{
		std::vector<int>{ 1, 2, 3, 4 }
    };

	int sum = 0;
	for ( const int value : object )
	{
		sum += value;
	}
	CHECK( sum == 10 );

	CHECK( object.size() == 4 );
	CHECK_FALSE( object.empty() );
	CHECK( std::ranges::count( object, 2 ) == 1 );
	CHECK( *std::ranges::max_element( object ) == 4 );
}

TEST_CASE( "strong_typedef range exposes size and empty for sized ranges", "[strong_typedef]" )
{
	const fixed_ints object{
		std::array<int, 3>{ 5, 6, 7 }
    };

	CHECK( object.size() == 3 );
	CHECK_FALSE( object.empty() );
	CHECK( std::ranges::begin( object ) != std::ranges::end( object ) );
}

// --- User-defined mixin -------------------------------------------------------------------------

namespace
{
	// A mixin defined outside the library following the same tag/mixin pattern, confirming has_mixin and the mixin
	// machinery are usable by downstream code. Mixins may add ordinary member functions, accessing the underlying value
	// through the derived type.
	struct squarable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr int square() const noexcept
			{
				const auto value = static_cast<const Derived&>( *this ).value;
				return value * value;
			}
		};
	};
}

using squared = mclo::strong_typedef<int, struct squared_tag, squarable>;

static_assert( mclo::has_mixin<squared, squarable> );
static_assert( !mclo::has_mixin<plain_int, squarable> );

TEST_CASE( "strong_typedef supports user-defined mixins", "[strong_typedef]" )
{
	CHECK( squared{ 7 }.square() == 49 );
}
