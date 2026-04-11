#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/flexible_array.hpp"

#include "mclo/allocator/arena_allocator.hpp"
#include "mclo/preprocessor/platform.hpp"

#include "assert_macros.hpp"

#include <array>
#include <forward_list>
#include <span>
#include <string>
#include <vector>

namespace
{
	struct counters
	{
		int constructions = 0;
		int destructions = 0;
		int copies = 0;

		static inline counters* current = nullptr;

		counters()
		{
			DEBUG_ASSERT( current == nullptr, "Only one counters instance should be active at a time" );
			current = this;
		}

		~counters()
		{
			current = nullptr;
		}

		void reset()
		{
			constructions = 0;
			destructions = 0;
			copies = 0;
		}

		counters( const counters& ) = delete;
		counters& operator=( const counters& ) = delete;
	};

	struct tracked
	{
		int value = 0;

		tracked()
		{
			DEBUG_ASSERT( counters::current, "No active counters instance" );
			++counters::current->constructions;
		}

		explicit tracked( const int v )
			: value( v )
		{
			DEBUG_ASSERT( counters::current, "No active counters instance" );
			++counters::current->constructions;
		}

		tracked( const tracked& other )
			: value( other.value )
		{
			DEBUG_ASSERT( counters::current, "No active counters instance" );
			++counters::current->constructions;
			++counters::current->copies;
		}

		~tracked()
		{
			DEBUG_ASSERT( counters::current, "No active counters instance" );
			++counters::current->destructions;
		}
	};

	MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( 4324 ) // structure was padded due to alignment specifier
	struct alignas( 32 ) over_aligned
	{
		int value = 0;
	};
	MCLO_MSVC_POP_WARNINGS
}

// Static assertions
static_assert( sizeof( mclo::flexible_array<int> ) == sizeof( void* ),
			   "Empty allocator should be optimized via no_unique_address" );

// Default construction

TEST_CASE( "flexible_array default construction", "[flexible_array]" )
{
	const mclo::flexible_array<int> arr;

	CHECK( arr.size() == 0 );
	CHECK( arr.data() == nullptr );
}

// Size construction

TEST_CASE( "flexible_array size construction", "[flexible_array]" )
{
	const mclo::flexible_array<int> arr( 5 );

	CHECK( arr.size() == 5 );
	REQUIRE( arr.data() != nullptr );
}

TEST_CASE( "flexible_array size zero construction", "[flexible_array]" )
{
	const mclo::flexible_array<int> arr( std::size_t( 0 ) );

	CHECK( arr.size() == 0 );
}

TEST_CASE( "flexible_array construction destroys on scope exit", "[flexible_array]" )
{
	counters c;

	{
		const mclo::flexible_array<tracked> arr( 3 );

		CHECK( c.constructions == 3 );
		CHECK( c.destructions == 0 );
	}

	CHECK( c.destructions == 3 );
}

// Allocator construction

TEST_CASE( "flexible_array allocator construction", "[flexible_array]" )
{
	mclo::memory_arena arena( 1024 );
	mclo::arena_allocator<int> allocator( arena );

	const mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> arr( allocator );

	CHECK( arr.size() == 0 );
	CHECK( arr.data() == nullptr );
	CHECK( arr.get_allocator() == arena );
}

TEST_CASE( "flexible_array size and allocator construction", "[flexible_array]" )
{
	mclo::memory_arena arena( 1024 );
	mclo::arena_allocator<int> allocator( arena );

	const mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> arr( 5, allocator );

	CHECK( arr.size() == 5 );
	REQUIRE( arr.data() != nullptr );
	CHECK( arr.get_allocator() == arena );
}

// Element access

TEST_CASE( "flexible_array operator[] read and write", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 3 );
	arr[ 0 ] = 10;
	arr[ 1 ] = 20;
	arr[ 2 ] = 30;

	CHECK( arr[ 0 ] == 10 );
	CHECK( arr[ 1 ] == 20 );
	CHECK( arr[ 2 ] == 30 );
}

TEST_CASE( "flexible_array const operator[]", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 3 );
	arr[ 0 ] = 42;

	const auto& const_arr = arr;

	CHECK( const_arr[ 0 ] == 42 );
}

TEST_CASE( "flexible_array operator[] out of bounds asserts", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 3 );

	CHECK_ASSERTS( (void)arr[ 3 ], "" );
}

// Data pointer

TEST_CASE( "flexible_array data returns mutable pointer", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );

	int* ptr = arr.data();
	REQUIRE( ptr != nullptr );
	ptr[ 0 ] = 99;

	CHECK( arr[ 0 ] == 99 );
}

TEST_CASE( "flexible_array const data returns const pointer", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );
	arr[ 0 ] = 55;

	const auto& const_arr = arr;
	const int* ptr = const_arr.data();

	REQUIRE( ptr != nullptr );
	CHECK( ptr[ 0 ] == 55 );
}

// Copy construction

TEST_CASE( "flexible_array copy construction empty", "[flexible_array]" )
{
	const mclo::flexible_array<int> original;

	const mclo::flexible_array<int> copy( original );

	CHECK( copy.size() == 0 );
	CHECK( copy.data() == nullptr );
}

TEST_CASE( "flexible_array copy construction copies elements", "[flexible_array]" )
{
	mclo::flexible_array<int> original( 3 );
	original[ 0 ] = 10;
	original[ 1 ] = 20;
	original[ 2 ] = 30;

	const mclo::flexible_array<int> copy( original );

	REQUIRE( copy.size() == 3 );
	REQUIRE( copy.data() != nullptr );
	CHECK( copy.data() != original.data() );
	CHECK( copy[ 0 ] == 10 );
	CHECK( copy[ 1 ] == 20 );
	CHECK( copy[ 2 ] == 30 );
}

TEST_CASE( "flexible_array copy construction with non trivial type", "[flexible_array]" )
{
	counters c;
	mclo::flexible_array<tracked> original( 2 );
	original[ 0 ].value = 100;
	original[ 1 ].value = 200;
	c.reset();

	const mclo::flexible_array<tracked> copy( original );

	REQUIRE( copy.size() == 2 );
	CHECK( copy[ 0 ].value == 100 );
	CHECK( copy[ 1 ].value == 200 );
	CHECK( c.copies == 2 );
}

// Copy assignment

TEST_CASE( "flexible_array copy assignment replaces contents", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );
	arr[ 0 ] = 1;
	arr[ 1 ] = 2;
	mclo::flexible_array<int> other( 3 );
	other[ 0 ] = 10;
	other[ 1 ] = 20;
	other[ 2 ] = 30;

	arr = other;

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == 10 );
	CHECK( arr[ 1 ] == 20 );
	CHECK( arr[ 2 ] == 30 );
	CHECK( arr.data() != other.data() );
}

TEST_CASE( "flexible_array copy self assignment is safe", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );
	arr[ 0 ] = 42;
	arr[ 1 ] = 43;

	arr = arr;

	CHECK( arr.size() == 2 );
	CHECK( arr[ 0 ] == 42 );
	CHECK( arr[ 1 ] == 43 );
}

TEST_CASE( "flexible_array copy assignment destroys old elements", "[flexible_array]" )
{
	counters c;
	mclo::flexible_array<tracked> arr( 3 );
	mclo::flexible_array<tracked> other( 1 );
	c.reset();

	arr = other;

	CHECK( c.destructions == 3 );
	CHECK( c.copies == 1 );
}

// Move construction

TEST_CASE( "flexible_array move construction transfers ownership", "[flexible_array]" )
{
	mclo::flexible_array<int> original( 3 );
	original[ 0 ] = 10;
	original[ 1 ] = 20;
	original[ 2 ] = 30;
	const int* original_data = original.data();

	mclo::flexible_array<int> moved( std::move( original ) );

	CHECK( moved.size() == 3 );
	CHECK( moved.data() == original_data );
	CHECK( moved[ 0 ] == 10 );
	CHECK( moved[ 1 ] == 20 );
	CHECK( moved[ 2 ] == 30 );
	CHECK( original.size() == 0 );
	CHECK( original.data() == nullptr );
}

TEST_CASE( "flexible_array move construction does not copy elements", "[flexible_array]" )
{
	counters c;
	mclo::flexible_array<tracked> original( 3 );
	c.reset();

	mclo::flexible_array<tracked> moved( std::move( original ) );

	CHECK( c.constructions == 0 );
	CHECK( c.copies == 0 );
}

// Move assignment

TEST_CASE( "flexible_array move assignment transfers ownership", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );
	arr[ 0 ] = 1;
	mclo::flexible_array<int> other( 3 );
	other[ 0 ] = 10;
	other[ 1 ] = 20;
	other[ 2 ] = 30;
	const int* other_data = other.data();

	arr = std::move( other );

	CHECK( arr.size() == 3 );
	CHECK( arr.data() == other_data );
	CHECK( arr[ 0 ] == 10 );
	CHECK( arr[ 1 ] == 20 );
	CHECK( arr[ 2 ] == 30 );
	CHECK( other.size() == 0 );
	CHECK( other.data() == nullptr );
}

TEST_CASE( "flexible_array move assignment destroys old elements", "[flexible_array]" )
{
	counters c;
	mclo::flexible_array<tracked> arr( 3 );
	mclo::flexible_array<tracked> other( 1 );
	c.reset();

	arr = std::move( other );

	CHECK( c.destructions == 3 );
	CHECK( c.copies == 0 );
}

TEST_CASE( "flexible_array move self assignment is safe", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );
	arr[ 0 ] = 42;

	arr = std::move( arr );

	CHECK( arr.size() == 2 );
	CHECK( arr[ 0 ] == 42 );
}

// Swap

TEST_CASE( "flexible_array member swap", "[flexible_array]" )
{
	mclo::flexible_array<int> a( 2 );
	a[ 0 ] = 1;
	a[ 1 ] = 2;
	mclo::flexible_array<int> b( 3 );
	b[ 0 ] = 10;
	b[ 1 ] = 20;
	b[ 2 ] = 30;
	const int* a_data = a.data();
	const int* b_data = b.data();

	a.swap( b );

	CHECK( a.size() == 3 );
	CHECK( a.data() == b_data );
	CHECK( a[ 0 ] == 10 );
	CHECK( b.size() == 2 );
	CHECK( b.data() == a_data );
	CHECK( b[ 0 ] == 1 );
}

TEST_CASE( "flexible_array ADL swap", "[flexible_array]" )
{
	mclo::flexible_array<int> a( 2 );
	a[ 0 ] = 1;
	mclo::flexible_array<int> b( 3 );
	b[ 0 ] = 10;

	using std::swap;
	swap( a, b );

	CHECK( a.size() == 3 );
	CHECK( a[ 0 ] == 10 );
	CHECK( b.size() == 2 );
	CHECK( b[ 0 ] == 1 );
}

TEST_CASE( "flexible_array self swap is safe", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 2 );
	arr[ 0 ] = 42;

	arr.swap( arr );

	CHECK( arr.size() == 2 );
	CHECK( arr[ 0 ] == 42 );
}

TEST_CASE( "flexible_array swap with empty", "[flexible_array]" )
{
	mclo::flexible_array<int> a( 3 );
	a[ 0 ] = 10;
	mclo::flexible_array<int> b;

	a.swap( b );

	CHECK( a.size() == 0 );
	CHECK( a.data() == nullptr );
	CHECK( b.size() == 3 );
	CHECK( b[ 0 ] == 10 );
}

// Alignment

TEST_CASE( "flexible_array over aligned type data is aligned", "[flexible_array]" )
{
	const mclo::flexible_array<over_aligned> arr( 4 );

	REQUIRE( arr.data() != nullptr );
	CHECK( reinterpret_cast<std::uintptr_t>( arr.data() ) % alignof( over_aligned ) == 0 );
}

TEST_CASE( "flexible_array char type with size_t header is aligned", "[flexible_array]" )
{
	const mclo::flexible_array<char> arr( 15 );

	REQUIRE( arr.data() != nullptr );
	CHECK( arr.size() == 15 );
}

// Custom size type

TEST_CASE( "flexible_array with uint16_t size type", "[flexible_array]" )
{
	mclo::flexible_array<int, std::uint16_t> arr( std::uint16_t( 5 ) );

	CHECK( arr.size() == 5 );
	arr[ 0 ] = 42;
	CHECK( arr[ 0 ] == 42 );
}

// Non-trivial types

TEST_CASE( "flexible_array with std::string", "[flexible_array]" )
{
	mclo::flexible_array<std::string> arr( 3 );
	arr[ 0 ] = "hello";
	arr[ 1 ] = "world";
	arr[ 2 ] = "test";

	CHECK( arr[ 0 ] == "hello" );
	CHECK( arr[ 1 ] == "world" );
	CHECK( arr[ 2 ] == "test" );
}

TEST_CASE( "flexible_array string copy construction deep copies", "[flexible_array]" )
{
	mclo::flexible_array<std::string> original( 2 );
	original[ 0 ] = "hello";
	original[ 1 ] = "world";

	mclo::flexible_array<std::string> copy( original );

	REQUIRE( copy.size() == 2 );
	CHECK( copy[ 0 ] == "hello" );
	CHECK( copy[ 1 ] == "world" );
	original[ 0 ] = "modified";
	CHECK( copy[ 0 ] == "hello" );
}

// Iterator pair construction

TEST_CASE( "flexible_array iterator pair construction from array", "[flexible_array]" )
{
	const std::array<int, 4> source = { 10, 20, 30, 40 };

	const mclo::flexible_array<int> arr( source.begin(), source.end() );

	REQUIRE( arr.size() == 4 );
	CHECK( arr[ 0 ] == 10 );
	CHECK( arr[ 1 ] == 20 );
	CHECK( arr[ 2 ] == 30 );
	CHECK( arr[ 3 ] == 40 );
}

TEST_CASE( "flexible_array iterator pair construction from vector", "[flexible_array]" )
{
	const std::vector<std::string> source = { "hello", "world", "test" };

	const mclo::flexible_array<std::string> arr( source.begin(), source.end() );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == "hello" );
	CHECK( arr[ 1 ] == "world" );
	CHECK( arr[ 2 ] == "test" );
}

TEST_CASE( "flexible_array iterator pair empty range", "[flexible_array]" )
{
	const std::vector<int> source;

	const mclo::flexible_array<int> arr( source.begin(), source.end() );

	CHECK( arr.size() == 0 );
	CHECK( arr.data() == nullptr );
}

TEST_CASE( "flexible_array iterator pair with allocator", "[flexible_array]" )
{
	mclo::memory_arena arena( 4096 );
	mclo::arena_allocator<int> allocator( arena );
	const std::array<int, 3> source = { 1, 2, 3 };

	const mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> arr(
		source.begin(), source.end(), allocator );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == 1 );
	CHECK( arr[ 1 ] == 2 );
	CHECK( arr[ 2 ] == 3 );
	CHECK( arr.get_allocator() == arena );
}

TEST_CASE( "flexible_array iterator pair from forward_list", "[flexible_array]" )
{
	const std::forward_list<int> source = { 5, 10, 15 };

	const mclo::flexible_array<int> arr( source.begin(), source.end() );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == 5 );
	CHECK( arr[ 1 ] == 10 );
	CHECK( arr[ 2 ] == 15 );
}

TEST_CASE( "flexible_array iterator pair constructs elements", "[flexible_array]" )
{
	counters c;
	mclo::flexible_array<tracked> source( 3 );
	source[ 0 ].value = 10;
	source[ 1 ].value = 20;
	source[ 2 ].value = 30;
	c.reset();

	const mclo::flexible_array<tracked> arr( source.data(), source.data() + source.size() );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ].value == 10 );
	CHECK( arr[ 1 ].value == 20 );
	CHECK( arr[ 2 ].value == 30 );
	CHECK( c.copies == 3 );
}

// Range construction

TEST_CASE( "flexible_array range construction from vector", "[flexible_array]" )
{
	const std::vector<int> source = { 1, 2, 3, 4, 5 };

	const mclo::flexible_array<int> arr( source );

	REQUIRE( arr.size() == 5 );
	CHECK( arr[ 0 ] == 1 );
	CHECK( arr[ 1 ] == 2 );
	CHECK( arr[ 2 ] == 3 );
	CHECK( arr[ 3 ] == 4 );
	CHECK( arr[ 4 ] == 5 );
}

TEST_CASE( "flexible_array range construction from array", "[flexible_array]" )
{
	const std::array<int, 3> source = { 10, 20, 30 };

	const mclo::flexible_array<int> arr( source );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == 10 );
	CHECK( arr[ 1 ] == 20 );
	CHECK( arr[ 2 ] == 30 );
}

TEST_CASE( "flexible_array range construction from span", "[flexible_array]" )
{
	const int raw[] = { 7, 8, 9 };
	const std::span<const int> source( raw );

	const mclo::flexible_array<int> arr( source );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == 7 );
	CHECK( arr[ 1 ] == 8 );
	CHECK( arr[ 2 ] == 9 );
}

TEST_CASE( "flexible_array range construction empty range", "[flexible_array]" )
{
	const std::vector<int> source;

	const mclo::flexible_array<int> arr( source );

	CHECK( arr.size() == 0 );
	CHECK( arr.data() == nullptr );
}

TEST_CASE( "flexible_array range construction with allocator", "[flexible_array]" )
{
	mclo::memory_arena arena( 4096 );
	mclo::arena_allocator<int> allocator( arena );
	const std::vector<int> source = { 1, 2, 3 };

	const mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> arr( source, allocator );

	REQUIRE( arr.size() == 3 );
	CHECK( arr[ 0 ] == 1 );
	CHECK( arr[ 1 ] == 2 );
	CHECK( arr[ 2 ] == 3 );
	CHECK( arr.get_allocator() == arena );
}

TEST_CASE( "flexible_array range construction from forward_list", "[flexible_array]" )
{
	const std::forward_list<int> source = { 100, 200 };

	const mclo::flexible_array<int> arr( source );

	REQUIRE( arr.size() == 2 );
	CHECK( arr[ 0 ] == 100 );
	CHECK( arr[ 1 ] == 200 );
}

// Custom allocator

TEST_CASE( "flexible_array with arena allocator", "[flexible_array]" )
{
	mclo::memory_arena arena( 4096 );
	mclo::arena_allocator<int> allocator( arena );

	mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> arr( 5, allocator );
	arr[ 0 ] = 42;

	CHECK( arr.size() == 5 );
	CHECK( arr[ 0 ] == 42 );
	CHECK( arr.get_allocator() == arena );
}

TEST_CASE( "flexible_array copy with arena allocator uses select_on_copy", "[flexible_array]" )
{
	mclo::memory_arena arena( 4096 );
	mclo::arena_allocator<int> allocator( arena );
	mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> original( 3, allocator );
	original[ 0 ] = 10;

	const auto copy( original );

	REQUIRE( copy.size() == 3 );
	CHECK( copy[ 0 ] == 10 );
	CHECK( copy.get_allocator() == arena );
}

TEST_CASE( "flexible_array move with arena allocator transfers allocator", "[flexible_array]" )
{
	mclo::memory_arena arena( 4096 );
	mclo::arena_allocator<int> allocator( arena );
	mclo::flexible_array<int, std::size_t, mclo::arena_allocator<int>> original( 3, allocator );
	original[ 0 ] = 10;
	const int* original_data = original.data();

	auto moved( std::move( original ) );

	CHECK( moved.size() == 3 );
	CHECK( moved.data() == original_data );
	CHECK( moved[ 0 ] == 10 );
	CHECK( moved.get_allocator() == arena );
	CHECK( original.size() == 0 );
}

// Destructor

TEST_CASE( "flexible_array destructor destroys all elements", "[flexible_array]" )
{
	counters c;

	{
		mclo::flexible_array<tracked> arr( 5 );
	}

	CHECK( c.constructions == 5 );
	CHECK( c.destructions == 5 );
}

TEST_CASE( "flexible_array destructor on empty is safe", "[flexible_array]" )
{
	{
		const mclo::flexible_array<int> arr;
	}
	// No crash = success
}

TEST_CASE( "flexible_array destructor on moved from is safe", "[flexible_array]" )
{
	mclo::flexible_array<int> arr( 3 );
	auto moved = std::move( arr );

	// arr destructor runs on moved-from state, should not crash
}
