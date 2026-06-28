#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/container/packed_int_array.hpp"
#include "mclo/meta/type_list.hpp"

namespace
{
	constexpr std::size_t test_size = 64;

	// clang-format off
	using test_types = mclo::meta::type_list<
		// Different bit widths
		mclo::packed_int_array<1, test_size, std::size_t>,
		mclo::packed_int_array<3, test_size, std::size_t>,
		mclo::packed_int_array<7, test_size, std::size_t>,
		mclo::packed_int_array<8, test_size, std::size_t>,
		mclo::packed_int_array<13, test_size, std::size_t>,
		// Different underlying types
		mclo::packed_int_array<7, test_size, std::uint8_t>,
		mclo::packed_int_array<7, test_size, std::uint16_t>,
		mclo::packed_int_array<7, test_size, std::uint32_t>,
		mclo::packed_int_array<7, test_size, std::uint64_t>,
		// BitWidth == bits_per_underlying (no packing)
		mclo::packed_int_array<8, test_size, std::uint8_t>,
		mclo::packed_int_array<32, test_size, std::uint32_t>
	>;
	// clang-format on
}

TEST_CASE( "packed_int_array max_value is correct", "[packed_int_array]" )
{
	static_assert( mclo::packed_int_array<7, 16, std::uint8_t>::max_value == 0b1111111 );
	static_assert( mclo::packed_int_array<7, 16, std::size_t>::max_value == 0b1111111 );
	static_assert( mclo::packed_int_array<18, 16, std::uint32_t>::max_value == 0x3FFFF );
	static_assert( mclo::packed_int_array<18, 16, std::size_t>::max_value == 0x3FFFF );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array default constructed, all elements are zero",
						 "[packed_int_array]",
						 test_types )
{
	TestType arr;

	CHECK( arr.size() == test_size );
	CHECK_FALSE( arr.empty() );
	for ( std::size_t i = 0; i < test_size; ++i )
	{
		CHECK( arr.get( i ) == 0 );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, size and empty", "[packed_int_array]", test_types )
{
	TestType arr;

	CHECK( arr.size() == test_size );
	CHECK_FALSE( arr.empty() );
}

TEST_CASE( "packed_int_array with zero size, is empty", "[packed_int_array]" )
{
	mclo::packed_int_array<7, 0, std::size_t> arr;

	CHECK( arr.size() == 0 );
	CHECK( arr.empty() );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array fill constructed, all elements have value",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr( max_val );

	for ( std::size_t i = 0; i < test_size; ++i )
	{
		CHECK( arr.get( i ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, set then get, round trips", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr;

	SECTION( "first element" )
	{
		arr.set( 0, static_cast<value_type>( 1 ) );
		CHECK( arr.get( 0 ) == 1 );
	}

	SECTION( "last element" )
	{
		arr.set( test_size - 1, max_val );
		CHECK( arr.get( test_size - 1 ) == max_val );
	}

	SECTION( "middle element" )
	{
		arr.set( test_size / 2, max_val );
		CHECK( arr.get( test_size / 2 ) == max_val );
	}

	SECTION( "multiple elements independently" )
	{
		const auto val1 = static_cast<value_type>( max_val > 1 ? 2 : 0 );
		arr.set( 0, static_cast<value_type>( 1 ) );
		arr.set( 1, val1 );
		arr.set( 2, max_val );

		CHECK( arr.get( 0 ) == 1 );
		CHECK( arr.get( 1 ) == val1 );
		CHECK( arr.get( 2 ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, set called twice, overwrites previous value",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr;

	arr.set( 3, max_val );
	CHECK( arr.get( 3 ) == max_val );

	arr.set( 3, static_cast<value_type>( 0 ) );
	CHECK( arr.get( 3 ) == 0 );

	arr.set( 3, static_cast<value_type>( 1 ) );
	CHECK( arr.get( 3 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, set one element, does not affect neighbors",
						 "[packed_int_array]",
						 test_types )
{
	constexpr auto max_val = TestType::max_value;

	TestType arr;

	arr.set( 7, max_val );

	CHECK( arr.get( 6 ) == 0 );
	CHECK( arr.get( 7 ) == max_val );
	CHECK( arr.get( 8 ) == 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, set ascending pattern, get returns correct values",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;
	const std::size_t mod = static_cast<std::size_t>( max_val ) + 1;

	TestType arr;

	for ( std::size_t i = 0; i < test_size; ++i )
	{
		arr.set( i, static_cast<value_type>( i % mod ) );
	}

	for ( std::size_t i = 0; i < test_size; ++i )
	{
		CHECK( arr.get( i ) == static_cast<value_type>( i % mod ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, front and back", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr;
	arr.set( 0, static_cast<value_type>( 1 ) );
	arr.set( test_size - 1, max_val );

	CHECK( arr.front() == 1 );
	CHECK( arr.back() == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, fill, sets all elements to value", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr;

	SECTION( "fill with max value" )
	{
		arr.fill( max_val );

		for ( std::size_t i = 0; i < test_size; ++i )
		{
			CHECK( arr.get( i ) == max_val );
		}
	}

	SECTION( "fill with zero after fill with max" )
	{
		arr.fill( max_val );
		arr.fill( static_cast<value_type>( 0 ) );

		for ( std::size_t i = 0; i < test_size; ++i )
		{
			CHECK( arr.get( i ) == 0 );
		}
	}

	SECTION( "fill with one" )
	{
		arr.fill( static_cast<value_type>( 1 ) );

		for ( std::size_t i = 0; i < test_size; ++i )
		{
			CHECK( arr.get( i ) == 1 );
		}
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, for_each, visits all elements in order", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;
	const std::size_t mod = static_cast<std::size_t>( max_val ) + 1;

	TestType arr;
	for ( std::size_t i = 0; i < test_size; ++i )
	{
		arr.set( i, static_cast<value_type>( i % mod ) );
	}

	std::size_t index = 0;
	arr.for_each( [ & ]( const value_type val ) {
		CHECK( val == static_cast<value_type>( index % mod ) );
		++index;
	} );

	CHECK( index == test_size );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, exchange, returns old value and sets new",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr;
	arr.set( 3, max_val );

	const auto old = arr.exchange( 3, static_cast<value_type>( 1 ) );

	CHECK( old == max_val );
	CHECK( arr.get( 3 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, exchange, does not affect neighbors", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType arr( max_val );

	static_cast<void>( arr.exchange( 4, static_cast<value_type>( 0 ) ) );

	CHECK( arr.get( 3 ) == max_val );
	CHECK( arr.get( 4 ) == 0 );
	CHECK( arr.get( 5 ) == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, copy and move", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType original;
	original.set( 0, max_val );
	original.set( test_size / 2, static_cast<value_type>( 1 ) );

	SECTION( "copy constructor" )
	{
		TestType copy( original );

		CHECK( copy.get( 0 ) == max_val );
		CHECK( copy.get( test_size / 2 ) == 1 );
		CHECK( copy == original );
	}

	SECTION( "copy assignment" )
	{
		TestType other;
		other = original;

		CHECK( other.get( 0 ) == max_val );
		CHECK( other.get( test_size / 2 ) == 1 );
		CHECK( other == original );
	}

	SECTION( "move constructor" )
	{
		TestType moved( std::move( original ) );

		CHECK( moved.get( 0 ) == max_val );
		CHECK( moved.get( test_size / 2 ) == 1 );
	}

	SECTION( "move assignment" )
	{
		TestType other;
		other = std::move( original );

		CHECK( other.get( 0 ) == max_val );
		CHECK( other.get( test_size / 2 ) == 1 );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, swap", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType a( max_val );
	TestType b;
	b.set( 0, static_cast<value_type>( 1 ) );

	SECTION( "member swap" )
	{
		a.swap( b );

		CHECK( a.get( 0 ) == 1 );
		CHECK( a.get( 1 ) == 0 );
		CHECK( b.get( 0 ) == max_val );
		CHECK( b.get( 1 ) == max_val );
	}

	SECTION( "free swap" )
	{
		swap( a, b );

		CHECK( a.get( 0 ) == 1 );
		CHECK( a.get( 1 ) == 0 );
		CHECK( b.get( 0 ) == max_val );
		CHECK( b.get( 1 ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, initializer list", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	std::initializer_list<value_type> init = [ & ] {
		// Build a test_size-element init list matching test_size
		static const auto values = [] {
			std::array<value_type, test_size> arr{};
			for ( std::size_t i = 0; i < test_size; ++i )
			{
				arr[ i ] = static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) );
			}
			return arr;
		}();
		return std::initializer_list<value_type>( values.data(), values.data() + values.size() );
	}();

	TestType arr( init );

	for ( std::size_t i = 0; i < test_size; ++i )
	{
		CHECK( arr.get( i ) == static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}
}

// Comparison tests

TEMPLATE_LIST_TEST_CASE( "packed_int_array, equality, default constructed are equal", "[packed_int_array]", test_types )
{
	TestType a;
	TestType b;

	CHECK( a == b );
	CHECK_FALSE( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, equality, filled containers are equal", "[packed_int_array]", test_types )
{
	constexpr auto max_val = TestType::max_value;

	TestType a( max_val );
	TestType b( max_val );

	CHECK( a == b );
	CHECK_FALSE( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, equality, same values set individually are equal",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType a;
	TestType b;
	for ( std::size_t i = 0; i < test_size; ++i )
	{
		const auto val = static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) );
		a.set( i, val );
		b.set( i, val );
	}

	CHECK( a == b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array filled, single element differs, not equal",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType a( max_val );
	TestType b( max_val );

	std::size_t index = 0;
	SECTION( "first element" )
	{
		index = 0;
	}
	SECTION( "middle element" )
	{
		index = test_size / 2;
	}
	SECTION( "last element" )
	{
		index = test_size - 1;
	}

	b.set( index, static_cast<value_type>( 0 ) );

	CHECK_FALSE( a == b );
	CHECK( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, spaceship, equal containers", "[packed_int_array]", test_types )
{
	constexpr auto max_val = TestType::max_value;

	TestType a( max_val );
	TestType b( max_val );

	CHECK( a <= b );
	CHECK( a >= b );
	CHECK_FALSE( a < b );
	CHECK_FALSE( a > b );
	CHECK( ( a <=> b ) == std::strong_ordering::equal );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, spaceship, first element differs", "[packed_int_array]", test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType a;
	TestType b;

	SECTION( "less" )
	{
		if constexpr ( max_val > 1 )
		{
			a.set( 0, static_cast<value_type>( 1 ) );
			b.set( 0, static_cast<value_type>( 2 ) );

			CHECK( a < b );
			CHECK( ( a <=> b ) == std::strong_ordering::less );
		}
	}

	SECTION( "greater" )
	{
		a.set( 0, max_val );
		b.set( 0, static_cast<value_type>( 0 ) );

		CHECK( a > b );
		CHECK( ( a <=> b ) == std::strong_ordering::greater );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_array, spaceship, later element determines ordering",
						 "[packed_int_array]",
						 test_types )
{
	using value_type = typename TestType::value_type;
	constexpr auto max_val = TestType::max_value;

	TestType a;
	TestType b;
	for ( std::size_t i = 0; i < 7; ++i )
	{
		a.set( i, static_cast<value_type>( 1 ) );
		b.set( i, static_cast<value_type>( 1 ) );
	}
	a.set( 7, static_cast<value_type>( 0 ) );
	b.set( 7, max_val );

	CHECK( a < b );
	CHECK( ( a <=> b ) == std::strong_ordering::less );
}

// Constexpr tests

TEST_CASE( "packed_int_array, constexpr default construction is zero-initialized", "[packed_int_array]" )
{
	static constexpr mclo::packed_int_array<7, 8, std::uint8_t> arr;

	STATIC_CHECK( arr.size() == 8 );
	STATIC_CHECK( !arr.empty() );
	STATIC_CHECK( arr.get( 0 ) == 0 );
	STATIC_CHECK( arr.get( 7 ) == 0 );
	STATIC_CHECK( arr.front() == 0 );
	STATIC_CHECK( arr.back() == 0 );
}

TEST_CASE( "packed_int_array, constexpr fill construction", "[packed_int_array]" )
{
	static constexpr mclo::packed_int_array<3, 16, std::size_t> arr( 5 );

	STATIC_CHECK( arr.size() == 16 );
	STATIC_CHECK( arr.get( 0 ) == 5 );
	STATIC_CHECK( arr.get( 8 ) == 5 );
	STATIC_CHECK( arr.get( 15 ) == 5 );
	STATIC_CHECK( arr.front() == 5 );
	STATIC_CHECK( arr.back() == 5 );
}

TEST_CASE( "packed_int_array, constexpr set and get", "[packed_int_array]" )
{
	static constexpr auto arr = [] {
		mclo::packed_int_array<7, 4, std::uint32_t> a;
		a.set( 0, 1 );
		a.set( 1, 42 );
		a.set( 2, 127 );
		a.set( 3, 0 );
		return a;
	}();

	STATIC_CHECK( arr.get( 0 ) == 1 );
	STATIC_CHECK( arr.get( 1 ) == 42 );
	STATIC_CHECK( arr.get( 2 ) == 127 );
	STATIC_CHECK( arr.get( 3 ) == 0 );
}

TEST_CASE( "packed_int_array, constexpr exchange", "[packed_int_array]" )
{
	static constexpr auto result = [] {
		mclo::packed_int_array<8, 4, std::uint8_t> a( 10 );
		const auto old = a.exchange( 2, 99 );
		return std::pair{ old, a.get( 2 ) };
	}();

	STATIC_CHECK( result.first == 10 );
	STATIC_CHECK( result.second == 99 );
}

TEST_CASE( "packed_int_array, constexpr fill after set", "[packed_int_array]" )
{
	static constexpr auto arr = [] {
		mclo::packed_int_array<3, 8, std::size_t> a;
		a.set( 0, 1 );
		a.set( 4, 5 );
		a.fill( 7 );
		return a;
	}();

	STATIC_CHECK( arr.get( 0 ) == 7 );
	STATIC_CHECK( arr.get( 4 ) == 7 );
	STATIC_CHECK( arr.get( 7 ) == 7 );
}

TEST_CASE( "packed_int_array, constexpr for_each", "[packed_int_array]" )
{
	static constexpr auto sum = [] {
		mclo::packed_int_array<4, 4, std::uint16_t> a;
		a.set( 0, 1 );
		a.set( 1, 2 );
		a.set( 2, 3 );
		a.set( 3, 4 );
		std::uint16_t total = 0;
		a.for_each( [ & ]( std::uint8_t val ) { total += val; } );
		return total;
	}();

	STATIC_CHECK( sum == 10 );
}

TEST_CASE( "packed_int_array, constexpr values crossing physical boundaries", "[packed_int_array]" )
{
	static constexpr auto arr = [] {
		mclo::packed_int_array<3, 10, std::uint8_t> a;
		for ( std::size_t i = 0; i < 10; ++i )
		{
			a.set( i, static_cast<std::uint8_t>( i % 8 ) );
		}
		return a;
	}();

	STATIC_CHECK( arr.get( 0 ) == 0 );
	STATIC_CHECK( arr.get( 1 ) == 1 );
	STATIC_CHECK( arr.get( 2 ) == 2 );
	STATIC_CHECK( arr.get( 7 ) == 7 );
	STATIC_CHECK( arr.get( 8 ) == 0 );
	STATIC_CHECK( arr.get( 9 ) == 1 );
}

TEST_CASE( "packed_int_array, constexpr swap", "[packed_int_array]" )
{
	static constexpr auto result = [] {
		mclo::packed_int_array<8, 2, std::uint8_t> a( 10 );
		mclo::packed_int_array<8, 2, std::uint8_t> b( 20 );
		swap( a, b );
		return std::pair{ a.get( 0 ), b.get( 0 ) };
	}();

	STATIC_CHECK( result.first == 20 );
	STATIC_CHECK( result.second == 10 );
}

TEST_CASE( "packed_int_array, constexpr empty array", "[packed_int_array]" )
{
	static constexpr mclo::packed_int_array<7, 0, std::size_t> arr;

	STATIC_CHECK( arr.size() == 0 );
	STATIC_CHECK( arr.empty() );
}

TEST_CASE( "packed_int_array, constexpr equality", "[packed_int_array]" )
{
	SECTION( "equal" )
	{
		static constexpr auto result = [] {
			mclo::packed_int_array<7, 4, std::uint32_t> a;
			mclo::packed_int_array<7, 4, std::uint32_t> b;
			a.set( 0, 42 );
			a.set( 3, 100 );
			b.set( 0, 42 );
			b.set( 3, 100 );
			return a == b;
		}();

		STATIC_CHECK( result );
	}

	SECTION( "not equal" )
	{
		static constexpr auto result = [] {
			mclo::packed_int_array<7, 4, std::uint32_t> a;
			mclo::packed_int_array<7, 4, std::uint32_t> b;
			a.set( 0, 42 );
			b.set( 0, 43 );
			return a == b;
		}();

		STATIC_CHECK_FALSE( result );
	}
}

TEST_CASE( "packed_int_array, constexpr spaceship", "[packed_int_array]" )
{
	SECTION( "less" )
	{
		static constexpr auto result = [] {
			mclo::packed_int_array<3, 4, std::uint8_t> a;
			mclo::packed_int_array<3, 4, std::uint8_t> b;
			a.set( 0, 1 );
			b.set( 0, 2 );
			return a <=> b;
		}();

		STATIC_CHECK( result == std::strong_ordering::less );
	}

	SECTION( "equal" )
	{
		static constexpr auto result = [] {
			mclo::packed_int_array<3, 4, std::uint8_t> a( 5 );
			mclo::packed_int_array<3, 4, std::uint8_t> b( 5 );
			return a <=> b;
		}();

		STATIC_CHECK( result == std::strong_ordering::equal );
	}

	SECTION( "greater" )
	{
		static constexpr auto result = [] {
			mclo::packed_int_array<8, 2, std::uint8_t> a;
			mclo::packed_int_array<8, 2, std::uint8_t> b;
			a.set( 0, 255 );
			b.set( 0, 0 );
			return a <=> b;
		}();

		STATIC_CHECK( result == std::strong_ordering::greater );
	}
}
