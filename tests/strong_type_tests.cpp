#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

// Strong type headers
#include "mclo/strong_type/arithmetic.hpp"
#include "mclo/strong_type/bitwise.hpp"
#include "mclo/strong_type/boolean.hpp"
#include "mclo/strong_type/decrementable.hpp"
#include "mclo/strong_type/default_initialized.hpp"
#include "mclo/strong_type/difference.hpp"
#include "mclo/strong_type/equality_comparable.hpp"
#include "mclo/strong_type/formattable.hpp"
#include "mclo/strong_type/hashable.hpp"
#include "mclo/strong_type/immovable.hpp"
#include "mclo/strong_type/implicitly_convertible.hpp"
#include "mclo/strong_type/incrementable.hpp"
#include "mclo/strong_type/indexed.hpp"
#include "mclo/strong_type/invocable.hpp"
#include "mclo/strong_type/move_only.hpp"
#include "mclo/strong_type/ordered.hpp"
#include "mclo/strong_type/pointer.hpp"
#include "mclo/strong_type/range.hpp"
#include "mclo/strong_type/regular.hpp"
#include "mclo/strong_type/scalable.hpp"
#include "mclo/strong_type/semiregular.hpp"
#include "mclo/strong_type/streamable.hpp"
#include "mclo/strong_type/type.hpp"

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

using plain_int = mclo::strong_type::type<int, struct plain_int_tag>;

static_assert( std::is_constructible_v<plain_int, int> );
static_assert( !std::is_default_constructible_v<plain_int> );
static_assert( !std::is_convertible_v<int, plain_int> ); // explicit only
static_assert( !std::is_convertible_v<plain_int, int> ); // no implicit conversion out
static_assert( sizeof( plain_int ) == sizeof( int ) );
static_assert( std::is_same_v<plain_int::value_type, int> );

TEST_CASE( "strong_type explicit construction stores the value", "[strong_type]" )
{
	const plain_int object{ 42 };
	CHECK( object.value == 42 );
}

TEST_CASE( "strong_type value is mutable", "[strong_type]" )
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

using overhead_plain = mclo::strong_type::type<int, struct overhead_plain_tag>;
using overhead_arithmetic = mclo::strong_type::type<int, struct overhead_arithmetic_tag, mclo::strong_type::arithmetic>;
using overhead_ordered = mclo::strong_type::type<int, struct overhead_ordered_tag, mclo::strong_type::ordered>;
using overhead_equality =
	mclo::strong_type::type<int, struct overhead_equality_tag, mclo::strong_type::equality_comparable>;
using overhead_semiregular =
	mclo::strong_type::type<int, struct overhead_semiregular_tag, mclo::strong_type::semiregular>;
using overhead_regular = mclo::strong_type::type<int, struct overhead_regular_tag, mclo::strong_type::regular>;
using overhead_composed = mclo::strong_type::type<int,
												  struct overhead_composed_tag,
												  mclo::strong_type::arithmetic,
												  mclo::strong_type::ordered,
												  mclo::strong_type::semiregular,
												  mclo::strong_type::hashable>;

static_assert( zero_overhead<overhead_plain, int> );
static_assert( zero_overhead<overhead_arithmetic, int> );
static_assert( zero_overhead<overhead_ordered, int> );
static_assert( zero_overhead<overhead_equality, int> );
static_assert( zero_overhead<overhead_semiregular, int> );
static_assert( zero_overhead<overhead_regular, int> );
static_assert( zero_overhead<overhead_composed, int> );

// --- Default construction marker ----------------------------------------------------------------

using default_int = mclo::strong_type::type<int, struct default_int_tag, mclo::strong_type::default_initialized>;

static_assert( std::is_default_constructible_v<default_int> );
static_assert( std::is_trivially_default_constructible_v<default_int> );
static_assert( sizeof( default_int ) == sizeof( int ) );

TEST_CASE( "strong_type default_initialized value initialises the underlying", "[strong_type]" )
{
	const default_int object{};
	CHECK( object.value == 0 );
}

// --- Copy/move propagation ----------------------------------------------------------------------

using trivial_int = mclo::strong_type::type<int, struct trivial_int_tag, mclo::strong_type::default_initialized>;
using string_type =
	mclo::strong_type::type<std::string, struct string_type_tag, mclo::strong_type::default_initialized>;

static_assert( std::is_trivially_copyable_v<trivial_int> );
static_assert( std::is_trivially_copy_constructible_v<trivial_int> );
static_assert( std::is_trivially_move_constructible_v<trivial_int> );

static_assert( !std::is_trivially_copyable_v<string_type> );
static_assert( std::is_copy_constructible_v<string_type> );
static_assert( std::is_move_constructible_v<string_type> );
static_assert( std::is_nothrow_move_constructible_v<string_type> );

using move_only_string = mclo::strong_type::
	type<std::string, struct move_only_tag, mclo::strong_type::default_initialized, mclo::strong_type::move_only>;

static_assert( !std::is_copy_constructible_v<move_only_string> );
static_assert( !std::is_copy_assignable_v<move_only_string> );
static_assert( std::is_move_constructible_v<move_only_string> );
static_assert( std::is_move_assignable_v<move_only_string> );

using immovable_string = mclo::strong_type::
	type<std::string, struct immovable_tag, mclo::strong_type::default_initialized, mclo::strong_type::immovable>;

static_assert( !std::is_copy_constructible_v<immovable_string> );
static_assert( !std::is_move_constructible_v<immovable_string> );

// --- Swap ---------------------------------------------------------------------------------------

TEST_CASE( "strong_type swap exchanges values", "[strong_type]" )
{
	string_type lhs{ std::string{ "hello" } };
	string_type rhs{ std::string{ "world" } };

	using std::swap;
	swap( lhs, rhs );

	CHECK( lhs.value == "world" );
	CHECK( rhs.value == "hello" );
}

// --- Implicit conversion mixin ------------------------------------------------------------------

using convertible_int = mclo::strong_type::type<int, struct convertible_tag, mclo::strong_type::implicitly_convertible>;

static_assert( std::is_convertible_v<convertible_int, int> );
static_assert( std::is_nothrow_convertible_v<convertible_int, const int&> );

TEST_CASE( "strong_type implicitly_convertible converts to the underlying", "[strong_type]" )
{
	const convertible_int object{ 7 };
	const int converted = object;
	CHECK( converted == 7 );
}

TEST_CASE( "strong_type implicitly_convertible exposes a mutable reference", "[strong_type]" )
{
	convertible_int object{ 7 };
	int& ref = object;
	ref = 9;
	CHECK( object.value == 9 );
}

using convertible_to_long =
	mclo::strong_type::type<int, struct convertible_to_tag, mclo::strong_type::implicitly_convertible_to<long>>;

static_assert( std::is_convertible_v<convertible_to_long, long> );
static_assert( std::is_nothrow_convertible_v<convertible_to_long, long> );
static_assert( !std::is_convertible_v<convertible_to_long, std::string> );

TEST_CASE( "strong_type implicitly_convertible_to converts to a specific type", "[strong_type]" )
{
	const convertible_to_long object{ 7 };
	const long converted = object;
	CHECK( converted == 7L );
}

// --- Difference mixin ---------------------------------------------------------------------------

using offset = mclo::strong_type::type<std::ptrdiff_t, struct offset_tag>;
using position = mclo::strong_type::type<std::ptrdiff_t, struct position_tag, mclo::strong_type::difference<offset>>;

static_assert( noexcept( std::declval<position>() - std::declval<position>() ) );

TEST_CASE( "strong_type difference subtracts to a distinct type", "[strong_type]" )
{
	const position a{ 10 };
	const position b{ 4 };
	const offset diff = a - b;
	static_assert( std::is_same_v<decltype( a - b ), offset> );
	CHECK( diff.value == 6 );
}

// --- Comparison mixins --------------------------------------------------------------------------

using equatable_int = mclo::strong_type::type<int, struct equatable_tag, mclo::strong_type::equality_comparable>;
using ordered_int = mclo::strong_type::type<int, struct ordered_tag, mclo::strong_type::ordered>;

static_assert( std::equality_comparable<equatable_int> );
static_assert( !std::three_way_comparable<equatable_int> );
static_assert( std::three_way_comparable<ordered_int> );

static_assert( noexcept( std::declval<equatable_int>() == std::declval<equatable_int>() ) );
static_assert( noexcept( std::declval<ordered_int>() == std::declval<ordered_int>() ) );
static_assert( noexcept( std::declval<ordered_int>() <=> std::declval<ordered_int>() ) );

TEST_CASE( "strong_type equality_comparable compares equality", "[strong_type]" )
{
	CHECK( equatable_int{ 1 } == equatable_int{ 1 } );
	CHECK( equatable_int{ 1 } != equatable_int{ 2 } );
}

TEST_CASE( "strong_type ordered compares relationally", "[strong_type]" )
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

using synth_ordered = mclo::strong_type::type<only_less_than, struct synth_ordered_tag, mclo::strong_type::ordered>;

static_assert( std::three_way_comparable<synth_ordered> );
static_assert( std::is_same_v<std::compare_three_way_result_t<synth_ordered>, std::weak_ordering> );

TEST_CASE( "strong_type ordered synthesises ordering for less-than-only types", "[strong_type]" )
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

using equatable_with_int =
	mclo::strong_type::type<int, struct equatable_with_tag, mclo::strong_type::equality_comparable_with<int>>;
using ordered_with_int = mclo::strong_type::type<int, struct ordered_with_tag, mclo::strong_type::ordered_with<int>>;

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

TEST_CASE( "strong_type equality_comparable_with compares against another type", "[strong_type]" )
{
	const equatable_with_int object{ 5 };

	CHECK( object == 5 );
	CHECK( 5 == object );
	CHECK( object != 6 );
	CHECK( 6 != object );
}

TEST_CASE( "strong_type ordered_with compares against another type", "[strong_type]" )
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

using add_int = mclo::strong_type::type<int, struct add_tag, mclo::strong_type::addable>;
using sub_int = mclo::strong_type::type<int, struct sub_tag, mclo::strong_type::subtractable>;
using mul_int = mclo::strong_type::type<int, struct mul_tag, mclo::strong_type::multipliable>;
using div_int = mclo::strong_type::type<int, struct div_tag, mclo::strong_type::dividable>;
using mod_int = mclo::strong_type::type<int, struct mod_tag, mclo::strong_type::modulable>;
using neg_int = mclo::strong_type::type<int, struct neg_tag, mclo::strong_type::negatable>;
using inc_int = mclo::strong_type::type<int, struct inc_tag, mclo::strong_type::incrementable>;
using dec_int = mclo::strong_type::type<int, struct dec_tag, mclo::strong_type::decrementable>;

// Each component mixin contributes only its own behaviour and composes nothing else.
static_assert( mclo::strong_type::has_mixin<add_int, mclo::strong_type::addable> &&
			   !mclo::strong_type::has_mixin<add_int, mclo::strong_type::subtractable> );
static_assert( mclo::strong_type::has_mixin<sub_int, mclo::strong_type::subtractable> &&
			   !mclo::strong_type::has_mixin<sub_int, mclo::strong_type::addable> );
static_assert( mclo::strong_type::has_mixin<mul_int, mclo::strong_type::multipliable> &&
			   !mclo::strong_type::has_mixin<mul_int, mclo::strong_type::dividable> );
static_assert( mclo::strong_type::has_mixin<div_int, mclo::strong_type::dividable> &&
			   !mclo::strong_type::has_mixin<div_int, mclo::strong_type::multipliable> );
static_assert( mclo::strong_type::has_mixin<mod_int, mclo::strong_type::modulable> &&
			   !mclo::strong_type::has_mixin<mod_int, mclo::strong_type::addable> );
static_assert( mclo::strong_type::has_mixin<neg_int, mclo::strong_type::negatable> &&
			   !mclo::strong_type::has_mixin<neg_int, mclo::strong_type::subtractable> );
static_assert( mclo::strong_type::has_mixin<inc_int, mclo::strong_type::incrementable> &&
			   !mclo::strong_type::has_mixin<inc_int, mclo::strong_type::decrementable> );
static_assert( mclo::strong_type::has_mixin<dec_int, mclo::strong_type::decrementable> &&
			   !mclo::strong_type::has_mixin<dec_int, mclo::strong_type::incrementable> );

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

TEST_CASE( "strong_type addable adds and compound adds", "[strong_type]" )
{
	CHECK( ( add_int{ 12 } + add_int{ 4 } ).value == 16 );
	add_int a{ 10 };
	a += add_int{ 5 };
	CHECK( a.value == 15 );
}

TEST_CASE( "strong_type subtractable subtracts and compound subtracts", "[strong_type]" )
{
	CHECK( ( sub_int{ 12 } - sub_int{ 4 } ).value == 8 );
	sub_int a{ 12 };
	a -= sub_int{ 3 };
	CHECK( a.value == 9 );
}

TEST_CASE( "strong_type multipliable multiplies and compound multiplies", "[strong_type]" )
{
	CHECK( ( mul_int{ 12 } * mul_int{ 4 } ).value == 48 );
	mul_int a{ 6 };
	a *= mul_int{ 2 };
	CHECK( a.value == 12 );
}

TEST_CASE( "strong_type dividable divides and compound divides", "[strong_type]" )
{
	CHECK( ( div_int{ 12 } / div_int{ 4 } ).value == 3 );
	div_int a{ 24 };
	a /= div_int{ 4 };
	CHECK( a.value == 6 );
}

TEST_CASE( "strong_type modulable mods and compound mods", "[strong_type]" )
{
	CHECK( ( mod_int{ 13 } % mod_int{ 4 } ).value == 1 );
	mod_int a{ 14 };
	a %= mod_int{ 4 };
	CHECK( a.value == 2 );
}

TEST_CASE( "strong_type negatable negates", "[strong_type]" )
{
	CHECK( ( -neg_int{ 5 } ).value == -5 );
	CHECK( ( -neg_int{ -3 } ).value == 3 );
}

TEST_CASE( "strong_type incrementable pre and post increments", "[strong_type]" )
{
	inc_int a{ 5 };
	CHECK( ( ++a ).value == 6 );
	CHECK( ( a++ ).value == 6 );
	CHECK( a.value == 7 );
}

TEST_CASE( "strong_type decrementable pre and post decrements", "[strong_type]" )
{
	dec_int a{ 5 };
	CHECK( ( --a ).value == 4 );
	CHECK( ( a-- ).value == 4 );
	CHECK( a.value == 3 );
}

// --- Arithmetic bundle --------------------------------------------------------------------------

using math_int = mclo::strong_type::type<int, struct math_tag, mclo::strong_type::arithmetic>;

// The bundle composes every component mixin; validate the composition structurally via has_mixin.
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::addable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::subtractable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::multipliable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::dividable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::modulable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::negatable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::incrementable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::decrementable> );
static_assert( mclo::strong_type::has_mixin<math_int, mclo::strong_type::arithmetic> );
static_assert( !mclo::strong_type::has_mixin<plain_int, mclo::strong_type::addable> );

TEST_CASE( "strong_type arithmetic bundle exposes the full operator set", "[strong_type]" )
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

using throwing_strong =
	mclo::strong_type::type<throwing_int, struct throwing_tag, mclo::strong_type::addable, mclo::strong_type::ordered>;

static_assert( !noexcept( std::declval<throwing_strong>() + std::declval<throwing_strong>() ) );
static_assert( !noexcept( std::declval<throwing_strong>() == std::declval<throwing_strong>() ) );
static_assert( !noexcept( std::declval<throwing_strong>() <=> std::declval<throwing_strong>() ) );

// --- Bitwise and shift mixins -------------------------------------------------------------------

using bits = mclo::strong_type::type<unsigned, struct bits_tag, mclo::strong_type::bitwise>;

static_assert( noexcept( std::declval<bits>() & std::declval<bits>() ) );
static_assert( noexcept( std::declval<bits>() | std::declval<bits>() ) );
static_assert( noexcept( std::declval<bits>() ^ std::declval<bits>() ) );
static_assert( noexcept( ~std::declval<bits>() ) );

TEST_CASE( "strong_type bitwise operations", "[strong_type]" )
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

using shift_int = mclo::strong_type::type<unsigned, struct shift_tag, mclo::strong_type::bitwise>;

TEST_CASE( "strong_type bitwise shift operations", "[strong_type]" )
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

using hashable_int = mclo::strong_type::type<int, struct hashable_tag, mclo::strong_type::hashable>;

static_assert( std_hashable<hashable_int> );
static_assert( !std_hashable<plain_int> );
static_assert( mclo_hashable<hashable_int> );
static_assert( noexcept( std::hash<hashable_int>{}( std::declval<hashable_int>() ) ) );
static_assert( mclo::strong_type::has_mixin<hashable_int, mclo::strong_type::hashable> );

TEST_CASE( "strong_type hashable matches the underlying std::hash", "[strong_type]" )
{
	CHECK( std::hash<hashable_int>{}( hashable_int{ 13 } ) == std::hash<int>{}( 13 ) );
}

TEST_CASE( "strong_type hashable hash_append matches the underlying", "[strong_type]" )
{
	mclo::fnv1a_hasher object_hasher;
	hash_append( object_hasher, hashable_int{ 13 } );

	mclo::fnv1a_hasher value_hasher;
	hash_append( value_hasher, 13 );

	CHECK( object_hasher.finish() == value_hasher.finish() );
}

// --- Formattable and streamable mixins ----------------------------------------------------------

using formattable_int = mclo::strong_type::type<int, struct formattable_tag, mclo::strong_type::formattable>;

static_assert( noexcept( format_as( std::declval<formattable_int>() ) ) );

TEST_CASE( "strong_type formattable exposes value via format_as", "[strong_type]" )
{
	CHECK( format_as( formattable_int{ 99 } ) == 99 );
}

using ostreamable_int = mclo::strong_type::type<int, struct ostreamable_tag, mclo::strong_type::ostreamable>;

TEST_CASE( "strong_type ostreamable inserts the value into a stream", "[strong_type]" )
{
	std::ostringstream stream;
	stream << ostreamable_int{ 42 };
	CHECK( stream.str() == "42" );
}

using istreamable_int = mclo::strong_type::type<int, struct istreamable_tag, mclo::strong_type::istreamable>;

TEST_CASE( "strong_type istreamable extracts the value from a stream", "[strong_type]" )
{
	std::istringstream stream{ "42" };
	istreamable_int object{ 0 };
	stream >> object;
	CHECK( object.value == 42 );
}

using iostreamable_int = mclo::strong_type::type<int, struct iostreamable_tag, mclo::strong_type::iostreamable>;

static_assert( mclo::strong_type::has_mixin<iostreamable_int, mclo::strong_type::istreamable> );
static_assert( mclo::strong_type::has_mixin<iostreamable_int, mclo::strong_type::ostreamable> );
static_assert( mclo::strong_type::has_mixin<iostreamable_int, mclo::strong_type::iostreamable> );

TEST_CASE( "strong_type iostreamable round trips through a stream", "[strong_type]" )
{
	std::stringstream stream;
	stream << iostreamable_int{ 7 };

	iostreamable_int object{ 0 };
	stream >> object;
	CHECK( object.value == 7 );
}

// --- Preset bundles -----------------------------------------------------------------------------

using number = mclo::strong_type::type<int,
									   struct number_tag,
									   mclo::strong_type::arithmetic,
									   mclo::strong_type::ordered,
									   mclo::strong_type::hashable,
									   mclo::strong_type::default_initialized>;

static_assert( std::three_way_comparable<number> );
static_assert( std_hashable<number> );
static_assert( sizeof( number ) == sizeof( int ) );
static_assert( std::numeric_limits<number>::is_specialized );
static_assert( std::numeric_limits<number>::is_signed == std::numeric_limits<int>::is_signed );
static_assert( std::numeric_limits<number>::digits == std::numeric_limits<int>::digits );

TEST_CASE( "strong_type composes arithmetic, ordering and hashing", "[strong_type]" )
{
	const number a{ 3 };
	const number b{ 4 };

	CHECK( ( a + b ) == number{ 7 } );
	CHECK( a < b );
	CHECK( std::hash<number>{}( a ) == std::hash<int>{}( 3 ) );
}

TEST_CASE( "strong_type arithmetic specialises std::numeric_limits", "[strong_type]" )
{
	CHECK( std::numeric_limits<number>::min() == number{ std::numeric_limits<int>::min() } );
	CHECK( std::numeric_limits<number>::max() == number{ std::numeric_limits<int>::max() } );
	CHECK( std::numeric_limits<number>::lowest() == number{ std::numeric_limits<int>::lowest() } );
}

using semiregular_string = mclo::strong_type::type<std::string, struct semiregular_tag, mclo::strong_type::semiregular>;

static_assert( std::is_default_constructible_v<semiregular_string> );
static_assert( std::is_copy_constructible_v<semiregular_string> );
static_assert( std::is_move_constructible_v<semiregular_string> );
static_assert( !std::equality_comparable<semiregular_string> );

TEST_CASE( "strong_type semiregular bundle enables default construction", "[strong_type]" )
{
	const semiregular_string object{};
	CHECK( object.value.empty() );
}

using regular_string = mclo::strong_type::type<std::string, struct regular_tag, mclo::strong_type::regular>;

static_assert( std::is_default_constructible_v<regular_string> );
static_assert( std::equality_comparable<regular_string> );
static_assert( !std_hashable<regular_string> );
static_assert( mclo::strong_type::has_mixin<regular_string, mclo::strong_type::semiregular> );
static_assert( mclo::strong_type::has_mixin<regular_string, mclo::strong_type::equality_comparable> );
static_assert( mclo::strong_type::has_mixin<regular_string, mclo::strong_type::regular> );

TEST_CASE( "strong_type regular bundle composes default construction and equality", "[strong_type]" )
{
	CHECK( regular_string{} == regular_string{ std::string{} } );
	CHECK( regular_string{ std::string{ "a" } } == regular_string{ std::string{ "a" } } );
	CHECK( regular_string{ std::string{ "a" } } != regular_string{ std::string{ "b" } } );
}

// --- Boolean mixin ------------------------------------------------------------------------------

using boolean_int = mclo::strong_type::type<int, struct boolean_tag, mclo::strong_type::boolean>;

static_assert( !std::is_convertible_v<boolean_int, bool> ); // explicit only
static_assert( std::is_constructible_v<bool, boolean_int> );
static_assert( noexcept( static_cast<bool>( std::declval<boolean_int>() ) ) );

TEST_CASE( "strong_type boolean exposes explicit operator bool", "[strong_type]" )
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

using int_pointer = mclo::strong_type::type<int*, struct int_pointer_tag, mclo::strong_type::pointer>;

static_assert( noexcept( std::declval<int_pointer>() == nullptr ) );

TEST_CASE( "strong_type pointer exposes dereference, arrow and nullptr comparison", "[strong_type]" )
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

using int_adder =
	mclo::strong_type::type<std::function<int( int, int )>, struct int_adder_tag, mclo::strong_type::invocable>;

TEST_CASE( "strong_type invocable forwards to the wrapped callable", "[strong_type]" )
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

using counter_fn = mclo::strong_type::type<mutable_counter, struct counter_fn_tag, mclo::strong_type::invocable>;
using sink_fn = mclo::strong_type::type<sink, struct sink_fn_tag, mclo::strong_type::invocable>;

TEST_CASE( "strong_type invocable forwards through the non-const overload", "[strong_type]" )
{
	counter_fn object{ mutable_counter{} };
	CHECK( object( 3 ) == 3 );
	CHECK( object( 4 ) == 7 );
}

TEST_CASE( "strong_type invocable perfectly forwards arguments", "[strong_type]" )
{
	const sink_fn object{ sink{} };
	CHECK( object( std::make_unique<int>( 42 ) ) == 42 );
}

// --- Indexed mixin ------------------------------------------------------------------------------

using indexed_array =
	mclo::strong_type::type<std::array<int, 3>, struct indexed_array_tag, mclo::strong_type::indexed<std::size_t>>;

TEST_CASE( "strong_type indexed exposes a subscript operator", "[strong_type]" )
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

using meters = mclo::strong_type::type<double, struct meters_tag, mclo::strong_type::scalable_with<double>>;

static_assert( mclo::strong_type::has_mixin<meters, mclo::strong_type::scalable_with<double>> );
static_assert( noexcept( std::declval<meters>() * std::declval<double>() ) );
static_assert( noexcept( std::declval<double>() * std::declval<meters>() ) );
static_assert( noexcept( std::declval<meters>() / std::declval<double>() ) );

// Scaling is only against the scalar; it does not introduce a strong * strong product.
static_assert( !mclo::strong_type::has_mixin<meters, mclo::strong_type::multipliable> );

TEST_CASE( "strong_type scalable_with scales by a scalar on either side", "[strong_type]" )
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

using int_vector = mclo::strong_type::
	type<std::vector<int>, struct int_vector_tag, mclo::strong_type::range, mclo::strong_type::semiregular>;
using fixed_ints = mclo::strong_type::type<std::array<int, 3>, struct fixed_ints_tag, mclo::strong_type::range>;
using int_list = mclo::strong_type::type<std::forward_list<int>, struct int_list_tag, mclo::strong_type::range>;

static_assert( std::ranges::range<int_vector> );
static_assert( std::ranges::range<const int_vector> );
static_assert( std::ranges::sized_range<int_vector> );

// A non-sized underlying range still models a range, but does not gain size() / empty().
static_assert( std::ranges::range<int_list> );
static_assert( !std::ranges::sized_range<int_list> );

TEST_CASE( "strong_type range iterates and dispatches to ranges algorithms", "[strong_type]" )
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

TEST_CASE( "strong_type range exposes size and empty for sized ranges", "[strong_type]" )
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

using squared = mclo::strong_type::type<int, struct squared_tag, squarable>;

static_assert( mclo::strong_type::has_mixin<squared, squarable> );
static_assert( !mclo::strong_type::has_mixin<plain_int, squarable> );

TEST_CASE( "strong_type supports user-defined mixins", "[strong_type]" )
{
	CHECK( squared{ 7 }.square() == 49 );
}
