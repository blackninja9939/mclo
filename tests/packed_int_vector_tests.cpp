#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mclo/container/packed_int_vector.hpp"
#include "mclo/container/small_vector.hpp"
#include "mclo/meta/type_list.hpp"

namespace
{
	template <typename ContainerType>
	struct test_wrapper
	{
		using vec_type = ContainerType;
		using value_type = typename ContainerType::value_type;
		static constexpr std::size_t bit_width = ContainerType::bit_width;
	};

	// clang-format off
	using test_types = mclo::meta::type_list<
		// Different bit widths with default std::vector<std::size_t>
		test_wrapper<mclo::packed_int_vector<1>>,
		test_wrapper<mclo::packed_int_vector<3>>,
		test_wrapper<mclo::packed_int_vector<7>>,
		test_wrapper<mclo::packed_int_vector<8>>,
		test_wrapper<mclo::packed_int_vector<13>>,
		// Different underlying types
		test_wrapper<mclo::packed_int_vector<7, std::uint8_t>>,
		test_wrapper<mclo::packed_int_vector<7, std::uint16_t>>,
		test_wrapper<mclo::packed_int_vector<7, std::uint32_t>>,
		test_wrapper<mclo::packed_int_vector<7, std::uint64_t>>,
		// BitWidth == bits_per_underlying (no packing)
		test_wrapper<mclo::packed_int_vector<8, std::uint8_t>>,
		test_wrapper<mclo::packed_int_vector<32, std::uint32_t>>,
		// small_vector as underlying container
		test_wrapper<mclo::packed_int_vector<7, std::uint32_t, mclo::small_vector<std::uint32_t, 8>>>,
		test_wrapper<mclo::packed_int_vector<3, std::uint64_t, mclo::small_vector<std::uint64_t, 4>>>
	>;
	// clang-format on
}

TEST_CASE( "packed_int_vector max_value is correct for specific configurations", "[packed_int_vector]" )
{
	// 7-bit max = 0b1111111 = 127
	static_assert( mclo::packed_int_vector<7, std::uint8_t>::max_value == 0b1111111 );
	static_assert( mclo::packed_int_vector<7, std::size_t>::max_value == 0b1111111 );
	static_assert( mclo::packed_int_vector<7, std::size_t, mclo::small_vector<std::size_t, 4>>::max_value ==
				   0b1111111 );

	// 18-bit max = 0x3FFFF = 262143
	static_assert( mclo::packed_int_vector<18, std::uint32_t>::max_value == 0x3FFFF );
	static_assert( mclo::packed_int_vector<18, std::size_t>::max_value == 0x3FFFF );
	static_assert( mclo::packed_int_vector<18, std::size_t, mclo::small_vector<std::size_t, 4>>::max_value == 0x3FFFF );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector constructed with size, has correct size",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec( 100 );

	CHECK( vec.size() == 100 );
	CHECK( vec.capacity() >= 100 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector default initialized, get returns zero", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec( 16 );

	for ( std::size_t i = 0; i < 16; ++i )
	{
		CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == 0 );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, set then get, round trips", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 32 );

	SECTION( "set first element" )
	{
		vec.set( 0, static_cast<value_type>( 1 ) );

		CHECK( vec.get( 0 ) == 1 );
	}

	SECTION( "set last element" )
	{
		vec.set( 31, max_val );

		CHECK( vec.get( 31 ) == max_val );
	}

	SECTION( "set other element" )
	{
		vec.set( 5, max_val );

		CHECK( vec.get( 5 ) == max_val );
	}

	SECTION( "set multiple elements independently" )
	{
		const auto val1 = static_cast<value_type>( max_val > 1 ? 2 : 0 );
		vec.set( 0, static_cast<value_type>( 1 ) );
		vec.set( 1, val1 );
		vec.set( 2, max_val );

		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 1 ) == val1 );
		CHECK( vec.get( 2 ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, set called twice, overwrites previous value",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8 );

	vec.set( 3, max_val );
	CHECK( vec.get( 3 ) == max_val );

	vec.set( 3, static_cast<value_type>( 0 ) );
	CHECK( vec.get( 3 ) == 0 );

	vec.set( 3, static_cast<value_type>( 1 ) );
	CHECK( vec.get( 3 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, set one element, does not affect neighbors",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 16 );

	vec.set( 7, max_val );

	CHECK( vec.get( 6 ) == 0 );
	CHECK( vec.get( 7 ) == max_val );
	CHECK( vec.get( 8 ) == 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, set ascending pattern, get returns correct values",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;
	constexpr std::size_t count = 64;

	vec_type vec( static_cast<typename vec_type::size_type>( count ) );

	for ( std::size_t i = 0; i < count; ++i )
	{
		vec.set( static_cast<typename vec_type::size_type>( i ),
				 static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}

	for ( std::size_t i = 0; i < count; ++i )
	{
		CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) ==
			   static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector constructed, max_size is positive", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec( 1 );

	CHECK( vec.max_size() > 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, copy constructor", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type original( 8 );
	original.set( 0, max_val );
	original.set( 4, max_val );

	vec_type copy( original );

	CHECK( copy.size() == 8 );
	CHECK( copy.get( 0 ) == max_val );
	CHECK( copy.get( 4 ) == max_val );
	CHECK( copy.get( 1 ) == 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, copy assignment", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type original( 8 );
	original.set( 2, max_val );

	vec_type other( 4 );
	other = original;

	CHECK( other.size() == 8 );
	CHECK( other.get( 2 ) == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, move constructor", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type original( 8 );
	original.set( 3, max_val );

	vec_type moved( std::move( original ) );

	CHECK( moved.size() == 8 );
	CHECK( moved.get( 3 ) == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, move assignment", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type original( 8 );
	original.set( 5, max_val );

	vec_type other( 4 );
	other = std::move( original );

	CHECK( other.size() == 8 );
	CHECK( other.get( 5 ) == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, member swap", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type original( 8 );
	original.set( 3, max_val );
	vec_type other( 4 );
	other.set( 3, static_cast<value_type>( 1 ) );

	original.swap( other );

	CHECK( other.size() == 8 );
	CHECK( other.get( 3 ) == max_val );
	CHECK( original.size() == 4 );
	CHECK( original.get( 3 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, free swap", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type original( 8 );
	original.set( 3, max_val );
	vec_type other( 4 );
	other.set( 3, static_cast<value_type>( 1 ) );

	swap( original, other );

	CHECK( other.size() == 8 );
	CHECK( other.get( 3 ) == max_val );
	CHECK( original.size() == 4 );
	CHECK( original.get( 3 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, fill, sets all elements to value", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	SECTION( "fill on empty vector is a no-op" )
	{
		vec_type vec;
		vec.fill( static_cast<value_type>( 1 ) );
		CHECK( vec.empty() );
	}

	constexpr std::size_t count = 64;

	vec_type vec( static_cast<typename vec_type::size_type>( count ) );

	SECTION( "fill with max value" )
	{
		vec.fill( max_val );

		for ( std::size_t i = 0; i < count; ++i )
		{
			CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == max_val );
		}
	}

	SECTION( "fill with zero" )
	{
		vec.fill( max_val );
		vec.fill( static_cast<value_type>( 0 ) );

		for ( std::size_t i = 0; i < count; ++i )
		{
			CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == 0 );
		}
	}

	SECTION( "fill with one" )
	{
		vec.fill( static_cast<value_type>( 1 ) );

		for ( std::size_t i = 0; i < count; ++i )
		{
			CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == 1 );
		}
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector default constructed, is empty", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec;

	CHECK( vec.empty() );
	CHECK( vec.size() == 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector constructed with size and value, all elements have value",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 16, max_val );

	CHECK( vec.size() == 16 );
	for ( std::size_t i = 0; i < 16; ++i )
	{
		CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, front and back, return correct values", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8 );
	vec.set( 0, static_cast<value_type>( 1 ) );
	vec.set( 7, max_val );

	CHECK( vec.front() == 1 );
	CHECK( vec.back() == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, push_back, appends elements", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec;

	vec.push_back( static_cast<value_type>( 1 ) );
	CHECK( vec.size() == 1 );
	CHECK( vec.capacity() >= 1 );
	CHECK( vec.get( 0 ) == 1 );

	vec.push_back( max_val );
	CHECK( vec.size() == 2 );
	CHECK( vec.capacity() >= 2 );
	CHECK( vec.get( 0 ) == 1 );
	CHECK( vec.get( 1 ) == max_val );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, push_back many elements, preserves values",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;
	constexpr std::size_t count = 64;

	vec_type vec;

	for ( std::size_t i = 0; i < count; ++i )
	{
		vec.push_back( static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}

	CHECK( vec.size() == count );
	for ( std::size_t i = 0; i < count; ++i )
	{
		CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) ==
			   static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, pop_back, removes last element", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 4 );
	vec.set( 0, static_cast<value_type>( 1 ) );
	vec.set( 1, max_val );
	vec.set( 2, static_cast<value_type>( 0 ) );
	vec.set( 3, static_cast<value_type>( 1 ) );

	vec.pop_back();

	CHECK( vec.size() == 3 );
	CHECK( vec.get( 0 ) == 1 );
	CHECK( vec.get( 1 ) == max_val );
	CHECK( vec.get( 2 ) == 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector constructed, capacity reflects physical storage",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec( 1 );

	CHECK( vec.capacity() >= vec.size() );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, reserve, increases capacity", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec;

	vec.reserve( 100 );

	CHECK( vec.capacity() >= 100 );
	CHECK( vec.size() == 0 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, resize from empty, fills with zero", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec;

	vec.resize( 16 );

	CHECK( vec.size() == 16 );
	CHECK( vec.capacity() >= 16 );
	for ( std::size_t i = 0; i < 16; ++i )
	{
		CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == 0 );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, resize from empty with value, fills with value",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec;

	vec.resize( 16, max_val );

	CHECK( vec.size() == 16 );
	CHECK( vec.capacity() >= 16 );
	for ( std::size_t i = 0; i < 16; ++i )
	{
		CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, resize", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8 );
	vec.set( 0, static_cast<value_type>( 1 ) );
	vec.set( 3, max_val );
	vec.set( 7, max_val );

	SECTION( "same size is a no-op" )
	{
		vec.resize( 8 );

		CHECK( vec.size() == 8 );
		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 3 ) == max_val );
		CHECK( vec.get( 7 ) == max_val );
	}

	SECTION( "grow fills new elements with zero" )
	{
		vec.resize( 12 );

		CHECK( vec.size() == 12 );
		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 3 ) == max_val );
		CHECK( vec.get( 7 ) == max_val );
		for ( std::size_t i = 8; i < 12; ++i )
		{
			CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == 0 );
		}
	}

	SECTION( "shrink preserves existing" )
	{
		vec.resize( 4 );

		CHECK( vec.size() == 4 );
		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 3 ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, resize with value", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8 );
	vec.set( 0, static_cast<value_type>( 1 ) );
	vec.set( 3, max_val );
	vec.set( 7, max_val );

	SECTION( "same size is a no-op" )
	{
		vec.resize( 8, static_cast<value_type>( 1 ) );

		CHECK( vec.size() == 8 );
		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 3 ) == max_val );
		CHECK( vec.get( 7 ) == max_val );
	}

	SECTION( "grow fills new elements with value" )
	{
		vec.resize( 12, max_val );

		CHECK( vec.size() == 12 );
		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 3 ) == max_val );
		CHECK( vec.get( 7 ) == max_val );
		for ( std::size_t i = 8; i < 12; ++i )
		{
			CHECK( vec.get( static_cast<typename vec_type::size_type>( i ) ) == max_val );
		}
	}

	SECTION( "shrink preserves existing" )
	{
		vec.resize( 4, max_val );

		CHECK( vec.size() == 4 );
		CHECK( vec.get( 0 ) == 1 );
		CHECK( vec.get( 3 ) == max_val );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, shrink_to_fit, reduces capacity", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec;
	vec.reserve( 100 );
	vec.push_back( typename TestType::value_type{ 1 } );

	const auto cap_before = vec.capacity();
	vec.shrink_to_fit();

	CHECK( vec.capacity() <= cap_before );
	CHECK( vec.size() == 1 );
	CHECK( vec.get( 0 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, clear, empties without changing capacity",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8, max_val );

	const auto cap_before = vec.capacity();
	vec.clear();

	CHECK( vec.empty() );
	CHECK( vec.size() == 0 );
	CHECK( vec.capacity() >= cap_before );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, for_each, visits all elements in order",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename vec_type::value_type;
	constexpr auto max_val = vec_type::max_value;

	const std::size_t mod = static_cast<std::size_t>( max_val ) + 1;

	vec_type vec( 64 );
	for ( std::size_t i = 0; i < 64; ++i )
	{
		vec.set( static_cast<typename vec_type::size_type>( i ), static_cast<value_type>( i % mod ) );
	}

	std::size_t index = 0;
	vec.for_each( [ & ]( const value_type val ) {
		CHECK( val == static_cast<value_type>( index % mod ) );
		++index;
	} );

	CHECK( index == 64 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, for_each on empty, is a no-op", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type vec;

	bool called = false;
	vec.for_each( [ & ]( typename vec_type::value_type ) { called = true; } );

	CHECK_FALSE( called );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, initializer list constructed, has correct values",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	SECTION( "single element" )
	{
		vec_type vec{ static_cast<value_type>( 1 ) };

		CHECK( vec.size() == 1 );
		CHECK( vec.get( 0 ) == 1 );
	}

	SECTION( "multiple elements" )
	{
		const auto mid = static_cast<value_type>( max_val / 2 );
		vec_type vec{ static_cast<value_type>( 0 ), static_cast<value_type>( 1 ), mid, max_val };

		CHECK( vec.size() == 4 );
		CHECK( vec.get( 0 ) == 0 );
		CHECK( vec.get( 1 ) == 1 );
		CHECK( vec.get( 2 ) == mid );
		CHECK( vec.get( 3 ) == max_val );
	}
}

TEST_CASE(
	"packed_int_vector<3, uint8_t>, set values crossing physical boundary, underlying matches expected representation",
	"[packed_int_vector]" )
{
	mclo::packed_int_vector<3, std::uint8_t> vec{ 0b010, 0b011, 0b101, 0b111 };

	const auto data = vec.underlying();

	REQUIRE( data.size() == 2 );
	CHECK( data[ 0 ] == 0b01011010 ); // v[2] low 2 bits | v[1] | v[0]
	CHECK( data[ 1 ] == 0b00001111 ); // padding | v[3] | v[2] high bit
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, exchange, returns old value and sets new",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8 );
	vec.set( 3, max_val );

	const auto old = vec.exchange( 3, static_cast<value_type>( 1 ) );

	CHECK( old == max_val );
	CHECK( vec.get( 3 ) == 1 );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, exchange, does not affect neighbors", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type vec( 8, max_val );

	static_cast<void>( vec.exchange( 4, static_cast<value_type>( 0 ) ) );

	CHECK( vec.get( 3 ) == max_val );
	CHECK( vec.get( 4 ) == 0 );
	CHECK( vec.get( 5 ) == max_val );
}

// Comparison tests

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, default constructed are equal",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type a( 8 );
	vec_type b( 8 );

	CHECK( a == b );
	CHECK_FALSE( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, filled containers are equal", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 8, max_val );
	vec_type b( 8, max_val );

	CHECK( a == b );
	CHECK_FALSE( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, same values set individually are equal",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 16 );
	vec_type b( 16 );
	for ( std::size_t i = 0; i < 16; ++i )
	{
		const auto val = static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) );
		a.set( static_cast<typename vec_type::size_type>( i ), val );
		b.set( static_cast<typename vec_type::size_type>( i ), val );
	}

	CHECK( a == b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector filled, single element differs, not equal",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 8, max_val );
	vec_type b( 8, max_val );

	std::size_t index = 0;
	SECTION( "first element" )
	{
		index = 0;
	}
	SECTION( "middle element" )
	{
		index = 4;
	}
	SECTION( "last element" )
	{
		index = 7;
	}

	b.set( static_cast<typename vec_type::size_type>( index ), static_cast<value_type>( 0 ) );

	CHECK_FALSE( a == b );
	CHECK( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, empty containers are equal", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type a;
	vec_type b;

	CHECK( a == b );
	CHECK_FALSE( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, different sizes compare not equal",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type a( 8 );
	vec_type b( 16 );

	CHECK_FALSE( a == b );
	CHECK( a != b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, resize down ignores trailing bits",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	// Build a with trailing dirty bits: fill 16 elements, then shrink to 8
	vec_type a( 16, max_val );
	a.resize( 8 );

	// Build b cleanly with 8 zero elements
	vec_type b( 8 );

	// The first 8 values of a were max_val, b's are zero, so they differ
	CHECK_FALSE( a == b );

	// Now set a's first 8 to zero so logical values match, despite a having dirty trailing bits
	for ( std::size_t i = 0; i < 8; ++i )
	{
		a.set( static_cast<typename vec_type::size_type>( i ), static_cast<value_type>( 0 ) );
	}

	CHECK( a == b );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, equality, resize down then compare with fresh container",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	// Fill 32 elements with an ascending pattern, then shrink to 16
	vec_type a( 32 );
	for ( std::size_t i = 0; i < 32; ++i )
	{
		a.set( static_cast<typename vec_type::size_type>( i ),
			   static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}
	a.resize( 16 );

	// Build the same 16 values from scratch
	vec_type b( 16 );
	for ( std::size_t i = 0; i < 16; ++i )
	{
		b.set( static_cast<typename vec_type::size_type>( i ),
			   static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}

	CHECK( a == b );
	CHECK( ( a <=> b ) == std::strong_ordering::equal );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, spaceship, equal containers", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 8, max_val );
	vec_type b( 8, max_val );

	CHECK( a <= b );
	CHECK( a >= b );
	CHECK_FALSE( a < b );
	CHECK_FALSE( a > b );
	CHECK( ( a <=> b ) == std::strong_ordering::equal );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, spaceship, first element differs", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 8 );
	vec_type b( 8 );

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

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, spaceship, later element determines ordering",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 8 );
	vec_type b( 8 );
	for ( std::size_t i = 0; i < 7; ++i )
	{
		a.set( static_cast<typename vec_type::size_type>( i ), static_cast<value_type>( 1 ) );
		b.set( static_cast<typename vec_type::size_type>( i ), static_cast<value_type>( 1 ) );
	}
	a.set( 7, static_cast<value_type>( 0 ) );
	b.set( 7, max_val );

	CHECK( a < b );
	CHECK( ( a <=> b ) == std::strong_ordering::less );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, spaceship, empty containers are equal", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;

	vec_type a;
	vec_type b;

	CHECK( ( a <=> b ) == std::strong_ordering::equal );
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, spaceship, different sizes", "[packed_int_vector]", test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	vec_type a( 4 );
	vec_type b( 8 );

	SECTION( "shorter is less when prefix matches" )
	{
		a.set( 0, static_cast<value_type>( 1 ) );
		b.set( 0, static_cast<value_type>( 1 ) );

		CHECK( a < b );
		CHECK( ( a <=> b ) == std::strong_ordering::less );
	}

	SECTION( "shorter can be greater by value" )
	{
		a.set( 0, max_val );
		b.set( 0, static_cast<value_type>( 0 ) );

		CHECK( a > b );
		CHECK( ( a <=> b ) == std::strong_ordering::greater );
	}
}

TEMPLATE_LIST_TEST_CASE( "packed_int_vector, spaceship, resize down ignores trailing bits",
						 "[packed_int_vector]",
						 test_types )
{
	using vec_type = typename TestType::vec_type;
	using value_type = typename TestType::value_type;
	constexpr auto max_val = vec_type::max_value;

	// Fill 16 with ascending, shrink to 8
	vec_type a( 16 );
	for ( std::size_t i = 0; i < 16; ++i )
	{
		a.set( static_cast<typename vec_type::size_type>( i ),
			   static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}
	a.resize( 8 );

	// Build same 8 values from scratch
	vec_type b( 8 );
	for ( std::size_t i = 0; i < 8; ++i )
	{
		b.set( static_cast<typename vec_type::size_type>( i ),
			   static_cast<value_type>( i % ( static_cast<std::size_t>( max_val ) + 1 ) ) );
	}

	CHECK( a == b );
	CHECK( ( a <=> b ) == std::strong_ordering::equal );
}

// Comparison optimization path coverage tests
// These use specific types to guarantee exercising the physical vs tail code paths,
// rather than relying on template instantiation choice.

// packed_int_vector<7, uint32_t> with 8 elements:
//   total_bits=56, bits_per_underlying=32, full_physical=1, tail_start=4
//   Physical comparison covers elements 0-3, tail covers elements 4-7.

TEST_CASE( "packed_int_vector, equality, mismatch in physical region", "[packed_int_vector]" )
{
	// full_physical=1, diff at element 0 → std::equal detects physical mismatch
	mclo::packed_int_vector<7, std::uint32_t> a( 8, 127 );
	mclo::packed_int_vector<7, std::uint32_t> b( 8, 127 );
	b.set( 0, 0 );

	CHECK_FALSE( a == b );
}

TEST_CASE( "packed_int_vector, equality, mismatch only in tail region", "[packed_int_vector]" )
{
	// full_physical=1, diff at element 6 → physical matches, tail get() detects mismatch
	mclo::packed_int_vector<7, std::uint32_t> a( 8, 127 );
	mclo::packed_int_vector<7, std::uint32_t> b( 8, 127 );
	b.set( 6, 0 );

	CHECK_FALSE( a == b );
}

TEST_CASE( "packed_int_vector, equality, no tail when exactly aligned", "[packed_int_vector]" )
{
	// packed_int_vector<8, uint8_t> with 8 elements: full_physical=8, tail_start=8
	// Pure physical comparison, no tail at all
	mclo::packed_int_vector<8, std::uint8_t> a( 8, 255 );
	mclo::packed_int_vector<8, std::uint8_t> b( 8, 255 );

	CHECK( a == b );

	b.set( 7, 0 );
	CHECK_FALSE( a == b );
}

TEST_CASE( "packed_int_vector, equality, diff in boundary-crossing element", "[packed_int_vector]" )
{
	// packed_int_vector<7, uint32_t> with 10 elements:
	//   full_physical = (10*7)/32 = 2, tail_start = (2*32)/7 = 9
	//   Element 4 occupies bits 28-34, crossing physical[0]→physical[1].
	//   Physical[0] still matches (bits 0-27 of element 4 are identical),
	//   std::equal detects diff at physical[1], so the fast path catches it.
	mclo::packed_int_vector<7, std::uint32_t> a( 10, 63 );
	mclo::packed_int_vector<7, std::uint32_t> b( 10, 63 );
	b.set( 4, 0 );

	CHECK_FALSE( a == b );
}

TEST_CASE( "packed_int_vector, spaceship, mismatch detected via physical skip", "[packed_int_vector]" )
{
	// packed_int_vector<8, uint8_t> with 8 elements: common_full_physical=8
	// std::mismatch skips equal physical elements then finds diff at physical[5]
	// logical_start = 5, comparison starts at element 5
	mclo::packed_int_vector<8, std::uint8_t> a( 8, 42 );
	mclo::packed_int_vector<8, std::uint8_t> b( 8, 42 );
	a.set( 5, 100 );
	b.set( 5, 50 );

	CHECK( a > b );
	CHECK( ( a <=> b ) == std::strong_ordering::greater );
}

TEST_CASE( "packed_int_vector, spaceship, physical matches and tail determines ordering", "[packed_int_vector]" )
{
	// packed_int_vector<7, uint32_t> with 8 elements: common_full_physical=1, tail_start=4
	// Physical element 0 matches, diff at element 6 (in tail)
	mclo::packed_int_vector<7, std::uint32_t> a( 8, 1 );
	mclo::packed_int_vector<7, std::uint32_t> b( 8, 1 );
	a.set( 6, 0 );
	b.set( 6, 100 );

	CHECK( a < b );
	CHECK( ( a <=> b ) == std::strong_ordering::less );
}

TEST_CASE( "packed_int_vector, spaceship, no tail when exactly aligned", "[packed_int_vector]" )
{
	// packed_int_vector<8, uint8_t> with 4 vs 8 elements: common_full_physical=4
	// All common physical match, ordering decided by size
	mclo::packed_int_vector<8, std::uint8_t> a( 4, 10 );
	mclo::packed_int_vector<8, std::uint8_t> b( 8, 10 );

	CHECK( a < b );
	CHECK( ( a <=> b ) == std::strong_ordering::less );
}

TEST_CASE( "packed_int_vector, spaceship, diff in boundary-crossing element", "[packed_int_vector]" )
{
	// packed_int_vector<7, uint32_t> with 10 elements:
	//   common_full_physical = (10*7)/32 = 2, tail_start = (2*32)/7 = 9
	//   Element 4 occupies bits 28-34, crossing physical boundary 0→1.
	//   Elements 0-8 match, element 9 differs (in the tail after two full physical elements).
	//   But the critical path: if element 4 differed, physical[0] would still match
	//   (element 4's high bits are in physical[1]), std::mismatch returns at physical[1],
	//   logical_start = 32/7 = 4, so element 4 is correctly included in the fallback loop.
	mclo::packed_int_vector<7, std::uint32_t> a( 10, 63 );
	mclo::packed_int_vector<7, std::uint32_t> b( 10, 63 );
	// Differ at element 4 which crosses the physical[0]→physical[1] boundary
	a.set( 4, 0 );

	CHECK( a < b );
	CHECK( ( a <=> b ) == std::strong_ordering::less );
}
