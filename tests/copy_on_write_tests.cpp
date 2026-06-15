#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/copy_on_write.hpp"

#include "mclo/allocator/arena_allocator.hpp"

#include <string>

namespace
{
	struct destroy_tracker
	{
		bool* m_destroyed;

		destroy_tracker( bool& destroyed )
			: m_destroyed( &destroyed )
		{
		}

		~destroy_tracker()
		{
			*m_destroyed = true;
		}
	};
}

// --- Construction ---

TEST_CASE( "default constructed copy_on_write, dereference, value is default constructed", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow;

	CHECK( *cow == 0 );
	CHECK_FALSE( cow.valueless_after_move() );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "value constructed copy_on_write, dereference, stores value", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow( 42 );

	CHECK( *cow == 42 );
	CHECK_FALSE( cow.valueless_after_move() );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "copy_on_write constructed with deduction guide, has deduced type", "[copy_on_write]" )
{
	mclo::copy_on_write cow{ 42 };

	CHECK( *cow == 42 );
	static_assert( std::same_as<decltype( cow ), mclo::copy_on_write<int>> );
}

TEST_CASE( "in place constructed copy_on_write, dereference, stores constructed value", "[copy_on_write]" )
{
	const mclo::copy_on_write<std::string> cow( std::in_place, 5, 'a' );

	CHECK( *cow == "aaaaa" );
	CHECK_FALSE( cow.valueless_after_move() );
}

TEST_CASE( "in place constructed copy_on_write with initializer list, dereference, stores constructed value",
		   "[copy_on_write]" )
{
	const mclo::copy_on_write<std::string> cow( std::in_place, { 'a', 'b', 'c', 'd', 'e' } );

	CHECK( *cow == "abcde" );
	CHECK_FALSE( cow.valueless_after_move() );
}

TEST_CASE( "copy_on_write, converting construction, constructs from convertible type", "[copy_on_write]" )
{
	const mclo::copy_on_write<std::string> cow( "hello" );

	CHECK( *cow == "hello" );
	CHECK( cow.use_count() == 1 );
}

// --- Allocator-extended construction ---

TEST_CASE( "copy_on_write, allocator default construct, stores allocator", "[copy_on_write]" )
{
	mclo::memory_arena arena( 256 );
	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow( std::allocator_arg, arena );

	CHECK( *cow == 0 );
	CHECK( cow.get_allocator() == arena );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "copy_on_write, allocator value construct, stores value and allocator", "[copy_on_write]" )
{
	mclo::memory_arena arena( 256 );
	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow( std::allocator_arg, arena, 42 );

	CHECK( *cow == 42 );
	CHECK( cow.get_allocator() == arena );
}

TEST_CASE( "copy_on_write, allocator in place construct, stores constructed value", "[copy_on_write]" )
{
	mclo::memory_arena arena( 256 );
	const mclo::copy_on_write<std::string, mclo::arena_allocator<std::string>> cow(
		std::allocator_arg, arena, std::in_place, 5, 'a' );

	CHECK( *cow == "aaaaa" );
	CHECK( cow.get_allocator() == arena );
}

TEST_CASE( "copy_on_write, allocator in place construct with initializer list, stores constructed value",
		   "[copy_on_write]" )
{
	mclo::memory_arena arena( 256 );
	const mclo::copy_on_write<std::string, mclo::arena_allocator<std::string>> cow(
		std::allocator_arg, arena, std::in_place, { 'a', 'b', 'c' } );

	CHECK( *cow == "abc" );
	CHECK( cow.get_allocator() == arena );
}

TEST_CASE( "copy_on_write, allocator copy construct same allocator, shares storage", "[copy_on_write]" )
{
	mclo::memory_arena arena( 256 );
	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow1( std::allocator_arg, arena, 42 );

	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow2( std::allocator_arg, arena, cow1 );

	CHECK( *cow2 == 42 );
	CHECK( cow1.use_count() == 2 );
	CHECK( cow1.identical_to( cow2 ) );
	CHECK( cow2.get_allocator() == arena );
}

TEST_CASE( "copy_on_write, allocator copy construct different allocator, deep copies", "[copy_on_write]" )
{
	mclo::memory_arena arena1( 256 );
	mclo::memory_arena arena2( 256 );
	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow1( std::allocator_arg, arena1, 42 );

	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow2( std::allocator_arg, arena2, cow1 );

	CHECK( *cow2 == 42 );
	CHECK( cow1.use_count() == 1 );
	CHECK( cow2.use_count() == 1 );
	CHECK_FALSE( cow1.identical_to( cow2 ) );
	CHECK( cow2.get_allocator() == arena2 );
}

TEST_CASE( "copy_on_write, allocator move construct same allocator, steals storage", "[copy_on_write]" )
{
	mclo::memory_arena arena( 256 );
	mclo::copy_on_write<int, mclo::arena_allocator<int>> cow1( std::allocator_arg, arena, 42 );

	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow2( std::allocator_arg, arena, std::move( cow1 ) );

	CHECK( *cow2 == 42 );
	CHECK( cow2.use_count() == 1 );
	CHECK( cow1.valueless_after_move() );
	CHECK( cow2.get_allocator() == arena );
}

TEST_CASE( "copy_on_write, allocator move construct different allocator, creates new storage", "[copy_on_write]" )
{
	mclo::memory_arena arena1( 256 );
	mclo::memory_arena arena2( 256 );
	mclo::copy_on_write<int, mclo::arena_allocator<int>> cow1( std::allocator_arg, arena1, 42 );

	const mclo::copy_on_write<int, mclo::arena_allocator<int>> cow2( std::allocator_arg, arena2, std::move( cow1 ) );

	CHECK( *cow2 == 42 );
	CHECK( cow2.use_count() == 1 );
	CHECK( cow1.valueless_after_move() );
	CHECK( cow2.get_allocator() == arena2 );
}

// --- Copy semantics (sharing) ---

TEST_CASE( "copy_on_write, copy construct from valueless, copy is valueless", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> moved = std::move( cow1 );
	CHECK( cow1.valueless_after_move() );

	const mclo::copy_on_write<int> cow2( cow1 );

	CHECK( cow2.valueless_after_move() );
}

TEST_CASE( "copy_on_write, copy construct, shares storage", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow1( 42 );

	const mclo::copy_on_write<int> cow2 = cow1;

	CHECK( *cow1 == 42 );
	CHECK( *cow2 == 42 );
	CHECK( cow1.use_count() == 2 );
	CHECK( cow2.use_count() == 2 );
	CHECK( cow1.identical_to( cow2 ) );
}

TEST_CASE( "independently constructed copy_on_writes, not identical", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow1( 42 );
	const mclo::copy_on_write<int> cow2( 42 );

	CHECK_FALSE( cow1.identical_to( cow2 ) );
}

TEST_CASE( "copy_on_write, copy assign, shares storage", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> cow2( 0 );

	cow2 = cow1;

	CHECK( *cow1 == 42 );
	CHECK( *cow2 == 42 );
	CHECK( cow1.use_count() == 2 );
	CHECK( cow2.use_count() == 2 );
	CHECK( cow1.identical_to( cow2 ) );
}

TEST_CASE( "copy_on_write, copy assign to self, no change", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 42 );

	const auto& ref = cow;
	cow = ref;

	CHECK( *cow == 42 );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "copy_on_write, copy assign from valueless, target becomes valueless", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> moved = std::move( cow1 );
	CHECK( cow1.valueless_after_move() );

	mclo::copy_on_write<int> cow2( 99 );
	cow2 = cow1;

	CHECK( cow2.valueless_after_move() );
}

TEST_CASE( "shared copy_on_write, copy assign over, old sharing group decrements", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 10 );
	const mclo::copy_on_write<int> cow1_copy = cow1;
	const mclo::copy_on_write<int> cow2( 20 );
	CHECK( cow1.use_count() == 2 );

	cow1 = cow2;

	CHECK( *cow1 == 20 );
	CHECK( cow1.identical_to( cow2 ) );
	CHECK( cow1_copy.use_count() == 1 );
	CHECK( cow2.use_count() == 2 );
}

TEST_CASE( "copy_on_write, multiple copies, all share storage", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow1( 99 );
	const mclo::copy_on_write<int> cow2 = cow1;
	const mclo::copy_on_write<int> cow3 = cow2;

	CHECK( cow1.use_count() == 3 );
	CHECK( cow2.use_count() == 3 );
	CHECK( cow3.use_count() == 3 );
	CHECK( cow1.identical_to( cow2 ) );
	CHECK( cow2.identical_to( cow3 ) );
}

TEST_CASE( "copy_on_write, use_count tracks through copy and destroy lifecycle", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	CHECK( cow1.use_count() == 1 );

	{
		const mclo::copy_on_write<int> cow2 = cow1;
		CHECK( cow1.use_count() == 2 );

		{
			const mclo::copy_on_write<int> cow3 = cow2;
			CHECK( cow1.use_count() == 3 );
		}

		CHECK( cow1.use_count() == 2 );
	}

	CHECK( cow1.use_count() == 1 );
}

// --- Move semantics ---

TEST_CASE( "copy_on_write, move construct, source is valueless", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 100 );

	mclo::copy_on_write<int> cow2 = std::move( cow1 );

	CHECK( *cow2 == 100 );
	CHECK( cow2.use_count() == 1 );
	CHECK( cow1.valueless_after_move() );
	CHECK_FALSE( cow2.valueless_after_move() );
}

TEST_CASE( "copy_on_write, move assign, source is valueless", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 75 );
	mclo::copy_on_write<int> cow2;

	cow2 = std::move( cow1 );

	CHECK( *cow2 == 75 );
	CHECK( cow1.valueless_after_move() );
	CHECK_FALSE( cow2.valueless_after_move() );
}

TEST_CASE( "shared copy_on_write, move assign over, old sharing group decrements", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 10 );
	const mclo::copy_on_write<int> cow1_copy = cow1;
	mclo::copy_on_write<int> cow2( 20 );
	CHECK( cow1.use_count() == 2 );

	cow1 = std::move( cow2 );

	CHECK( *cow1 == 20 );
	CHECK( cow1.use_count() == 1 );
	CHECK( cow1_copy.use_count() == 1 );
	CHECK( cow2.valueless_after_move() );
}

TEST_CASE( "valueless copy_on_write, move construct from valueless, both valueless", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> moved = std::move( cow1 );
	CHECK( cow1.valueless_after_move() );

	mclo::copy_on_write<int> cow2 = std::move( cow1 );

	CHECK( cow1.valueless_after_move() );
	CHECK( cow2.valueless_after_move() );
}

TEST_CASE( "valueless copy_on_write, move assign from valueless, both valueless", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> moved = std::move( cow1 );
	CHECK( cow1.valueless_after_move() );

	mclo::copy_on_write<int> cow2( 99 );
	cow2 = std::move( cow1 );

	CHECK( cow1.valueless_after_move() );
	CHECK( cow2.valueless_after_move() );
}

TEST_CASE( "copy_on_write, self move assign, no change", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 42 );

	cow = std::move( cow );

	CHECK( *cow == 42 );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "shared copy_on_write, move one, remaining copies unaffected", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	const mclo::copy_on_write<int> cow2 = cow1;
	CHECK( cow1.use_count() == 2 );

	mclo::copy_on_write<int> cow3 = std::move( cow1 );

	CHECK( cow1.valueless_after_move() );
	CHECK( *cow2 == 42 );
	CHECK( *cow3 == 42 );
	CHECK( cow2.use_count() == 2 );
	CHECK( cow3.use_count() == 2 );
	CHECK( cow2.identical_to( cow3 ) );
}

// --- modify (copy-on-write semantics) ---

TEST_CASE( "previously shared copy_on_write, becomes unique after copy destroyed, modify in place", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );

	{
		const mclo::copy_on_write<int> cow2 = cow1;
		CHECK( cow1.use_count() == 2 );
	}

	CHECK( cow1.use_count() == 1 );

	cow1.modify( []( int& v ) { v = 99; } );

	CHECK( *cow1 == 99 );
	CHECK( cow1.use_count() == 1 );
}

TEST_CASE( "unique copy_on_write, modify, modifies in place", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 10 );

	cow.modify( []( int& v ) { v += 5; } );

	CHECK( *cow == 15 );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "shared copy_on_write, modify, creates copy and modifies copy", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> cow2 = cow1;
	CHECK( cow1.identical_to( cow2 ) );

	cow1.modify( []( int& v ) { v = 100; } );

	CHECK( *cow1 == 100 );
	CHECK( *cow2 == 42 );
	CHECK( cow1.use_count() == 1 );
	CHECK( cow2.use_count() == 1 );
	CHECK_FALSE( cow1.identical_to( cow2 ) );
}

TEST_CASE( "shared copy_on_write, two-arg modify, does not modify in place", "[copy_on_write]" )
{
	mclo::copy_on_write<std::string> cow1( std::string( "hello" ) );
	mclo::copy_on_write<std::string> cow2 = cow1;
	CHECK( cow1.identical_to( cow2 ) );

	bool modified_in_place = false;
	bool constructed_new = false;

	cow1.modify(
		[ & ]( std::string& s ) {
			modified_in_place = true;
			s += " world";
		},
		[ & ]( const std::string& s ) -> std::string {
			constructed_new = true;
			return s + " world";
		} );

	CHECK( *cow1 == "hello world" );
	CHECK( *cow2 == "hello" );
	CHECK( constructed_new );
	CHECK_FALSE( modified_in_place );
}

TEST_CASE( "unique copy_on_write, two-arg modify, modifies in place", "[copy_on_write]" )
{
	mclo::copy_on_write<std::string> cow( std::string( "hello" ) );

	bool modified_in_place = false;
	bool constructed_new = false;

	cow.modify(
		[ & ]( std::string& s ) {
			modified_in_place = true;
			s += " world";
		},
		[ & ]( const std::string& s ) -> std::string {
			constructed_new = true;
			return s + " world";
		} );

	CHECK( *cow == "hello world" );
	CHECK( modified_in_place );
	CHECK_FALSE( constructed_new );
}

TEST_CASE( "three shared copy_on_writes, modify one, only modified creates copy", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 5 );
	const mclo::copy_on_write<int> cow2 = cow1;
	const mclo::copy_on_write<int> cow3 = cow1;
	CHECK( cow1.use_count() == 3 );

	cow1.modify( []( int& v ) { v = 99; } );

	CHECK( *cow1 == 99 );
	CHECK( *cow2 == 5 );
	CHECK( *cow3 == 5 );
	CHECK( cow1.use_count() == 1 );
	CHECK( cow2.use_count() == 2 );
	CHECK( cow3.use_count() == 2 );
	CHECK( cow2.identical_to( cow3 ) );
	CHECK_FALSE( cow1.identical_to( cow2 ) );
}

// --- Value assignment ---

TEST_CASE( "copy_on_write, converting value assignment, assigns convertible type", "[copy_on_write]" )
{
	mclo::copy_on_write<std::string> cow( std::string( "hello" ) );

	cow = "world";

	CHECK( *cow == "world" );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "unique copy_on_write, assign value, assigns in place", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 10 );

	cow = 20;

	CHECK( *cow == 20 );
	CHECK( cow.use_count() == 1 );
}

TEST_CASE( "shared copy_on_write, assign value, creates new storage", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 10 );
	const mclo::copy_on_write<int> cow2 = cow1;
	CHECK( cow1.identical_to( cow2 ) );

	cow1 = 20;

	CHECK( *cow1 == 20 );
	CHECK( *cow2 == 10 );
	CHECK( cow1.use_count() == 1 );
	CHECK( cow2.use_count() == 1 );
	CHECK_FALSE( cow1.identical_to( cow2 ) );
}

TEST_CASE( "valueless copy_on_write, assign value, creates new storage", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 10 );
	mclo::copy_on_write<int> moved = std::move( cow );
	CHECK( cow.valueless_after_move() );

	cow = 42;

	CHECK( *cow == 42 );
	CHECK_FALSE( cow.valueless_after_move() );
	CHECK( cow.use_count() == 1 );
}

// --- Swap ---

TEST_CASE( "two copy_on_writes, swap, values exchanged", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 10 );
	mclo::copy_on_write<int> cow2( 20 );

	SECTION( "member swap" )
	{
		cow1.swap( cow2 );

		CHECK( *cow1 == 20 );
		CHECK( *cow2 == 10 );
	}
	SECTION( "free function swap" )
	{
		swap( cow1, cow2 );

		CHECK( *cow1 == 20 );
		CHECK( *cow2 == 10 );
	}
}

TEST_CASE( "valueless and non-valueless copy_on_write, swap, states exchanged", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 30 );
	mclo::copy_on_write<int> cow2 = std::move( cow1 );
	CHECK( cow1.valueless_after_move() );

	using std::swap;
	swap( cow1, cow2 );

	CHECK( cow2.valueless_after_move() );
	CHECK( *cow1 == 30 );
}

TEST_CASE( "shared copy_on_write, swap one, sharing preserved", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 10 );
	const mclo::copy_on_write<int> cow1_copy = cow1;
	mclo::copy_on_write<int> cow2( 20 );
	CHECK( cow1.identical_to( cow1_copy ) );

	cow1.swap( cow2 );

	CHECK( *cow1 == 20 );
	CHECK( *cow2 == 10 );
	CHECK( cow2.identical_to( cow1_copy ) );
	CHECK( cow1_copy.use_count() == 2 );
}

// --- Comparison ---

TEST_CASE( "copy_on_write, compare equal", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow( 42 );

	SECTION( "same source compares equal" )
	{
		const mclo::copy_on_write<int> other = cow;
		CHECK( cow == other );
	}
	SECTION( "different source, same value, compares equal" )
	{
		const mclo::copy_on_write<int> other( 42 );
		CHECK( cow == other );
	}
	SECTION( "different source, different value, compares not equal" )
	{
		const mclo::copy_on_write<int> other( 99 );
		CHECK_FALSE( cow == other );
	}
	SECTION( "compare with same raw value, compares equal" )
	{
		CHECK( cow == 42 );
	}
	SECTION( "compare with different raw value, compares not equal" )
	{
		CHECK_FALSE( cow == 99 );
	}
}

TEST_CASE( "copy_on_write, three way compare", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow( 10 );

	SECTION( "same source compares equal" )
	{
		const mclo::copy_on_write<int> other = cow;
		CHECK( ( cow <=> other ) == std::strong_ordering::equal );
	}
	SECTION( "different source, same value, compares equal" )
	{
		const mclo::copy_on_write<int> other( 10 );
		CHECK( ( cow <=> other ) == std::strong_ordering::equal );
	}
	SECTION( "different source, lesser value, compares less" )
	{
		const mclo::copy_on_write<int> other( 20 );
		CHECK( ( cow <=> other ) == std::strong_ordering::less );
	}
	SECTION( "different source, greater value, compares greater" )
	{
		const mclo::copy_on_write<int> other( 20 );
		CHECK( ( other <=> cow ) == std::strong_ordering::greater );
	}
	SECTION( "compare with same raw value, compares equal" )
	{
		CHECK( ( cow <=> 10 ) == std::strong_ordering::equal );
	}
	SECTION( "compare with lesser raw value, compares greater" )
	{
		CHECK( ( cow <=> 5 ) == std::strong_ordering::greater );
	}
	SECTION( "compare with greater raw value, compares less" )
	{
		CHECK( ( cow <=> 20 ) == std::strong_ordering::less );
	}
}

TEST_CASE( "valueless copy_on_writes, compare, both valueless are equal", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow1( 42 );
	mclo::copy_on_write<int> cow2( 42 );
	mclo::copy_on_write<int> moved1 = std::move( cow1 );
	mclo::copy_on_write<int> moved2 = std::move( cow2 );

	CHECK( cow1 == cow2 );
	CHECK_FALSE( cow1 == moved1 );
	CHECK_FALSE( moved1 == cow1 );

	CHECK( ( cow1 <=> moved1 ) == std::strong_ordering::less );
	CHECK( ( moved1 <=> moved2 ) == std::strong_ordering::equal );
}

// --- Destruction ---

TEST_CASE( "unique copy_on_write, destroy, value is destroyed", "[copy_on_write]" )
{
	bool destroyed = false;

	{
		mclo::copy_on_write<destroy_tracker> cow( destroyed );
		CHECK_FALSE( destroyed );
	}

	CHECK( destroyed );
}

TEST_CASE( "shared copy_on_write, destroy one, value not destroyed until last", "[copy_on_write]" )
{
	bool destroyed = false;

	{
		mclo::copy_on_write<destroy_tracker> cow1( destroyed );
		{
			mclo::copy_on_write<destroy_tracker> cow2( cow1 );

			CHECK( cow1.use_count() == 2 );
			CHECK_FALSE( destroyed );
		}

		CHECK_FALSE( destroyed );
		CHECK( cow1.use_count() == 1 );
	}

	CHECK( destroyed );
}

// --- Hash ---

TEST_CASE( "copy_on_write, hash, matches hash of contained value", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow( 42 );
	const std::hash<mclo::copy_on_write<int>> hasher;

	CHECK( hasher( cow ) == std::hash<int>{}( 42 ) );
}

TEST_CASE( "valueless copy_on_write, hash, does not throw", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 42 );
	mclo::copy_on_write<int> moved = std::move( cow );

	const std::hash<mclo::copy_on_write<int>> hasher;

	static_cast<void>( hasher( cow ) );
}

// --- Dereference ---

TEST_CASE( "copy_on_write, arrow operator, accesses member", "[copy_on_write]" )
{
	const mclo::copy_on_write<std::string> cow( std::string( "hello" ) );

	CHECK( cow->size() == 5 );
}

TEST_CASE( "copy_on_write, dereference returns const reference", "[copy_on_write]" )
{
	mclo::copy_on_write<int> cow( 42 );

	static_assert( std::is_same_v<decltype( *cow ), const int&> );
}

// --- Allocator ---

TEST_CASE( "copy_on_write, get_allocator, returns allocator", "[copy_on_write]" )
{
	const mclo::copy_on_write<int> cow( 42 );

	[[maybe_unused]] const auto alloc = cow.get_allocator();
	static_assert( std::is_same_v<decltype( cow.get_allocator() ), std::allocator<int>> );
}

// --- Type traits ---

static_assert( std::is_copy_constructible_v<mclo::copy_on_write<int>> );
static_assert( std::is_copy_assignable_v<mclo::copy_on_write<int>> );
static_assert( std::is_nothrow_move_constructible_v<mclo::copy_on_write<int>> );
static_assert( std::is_nothrow_move_assignable_v<mclo::copy_on_write<int>> );
