#include <catch2/catch_test_macros.hpp>

#include "mclo/functional/move_only_function.hpp"

#include <array>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <type_traits>
#include <vector>

namespace
{
	struct multiplier
	{
		int factor;

		int operator()( const int value ) const
		{
			return value * factor;
		}
	};

	struct sum_list
	{
		std::vector<int> values;

		sum_list( const std::initializer_list<int> init )
			: values( init )
		{
		}

		int operator()() const
		{
			return std::accumulate( values.begin(), values.end(), 0 );
		}
	};
}

TEST_CASE( "default constructed move_only_function, when queried, then it is empty", "[move_only_function]" )
{
	// Given
	const mclo::move_only_function<int()> function;

	// When / Then
	CHECK_FALSE( static_cast<bool>( function ) );
	CHECK( function == nullptr );
}

TEST_CASE( "move_only_function holding a small callable, when invoked, then calls the target", "[move_only_function]" )
{
	// Given
	int captured = 10;
	mclo::move_only_function<int( int )> function = [ captured ]( const int value ) { return value + captured; };

	// When / Then
	REQUIRE( static_cast<bool>( function ) );
	CHECK( function( 5 ) == 15 );
}

TEST_CASE( "move_only_function holding a large callable, when invoked, then calls the heap-stored target",
		   "[move_only_function]" )
{
	// Given a callable far larger than the inline buffer
	std::array<int, 32> data;
	std::iota( data.begin(), data.end(), 1 );
	mclo::move_only_function<int()> function = [ data ]() { return std::accumulate( data.begin(), data.end(), 0 ); };

	// When / Then
	REQUIRE( static_cast<bool>( function ) );
	CHECK( function() == 528 );
}

TEST_CASE( "move_only_function holding a move-only target, when invoked, then calls it", "[move_only_function]" )
{
	// Given a lambda capturing a unique_ptr, which is not copyable
	auto owned = std::make_unique<int>( 7 );
	mclo::move_only_function<int()> function = [ owned = std::move( owned ) ]() { return *owned; };

	// When / Then
	REQUIRE( static_cast<bool>( function ) );
	CHECK( function() == 7 );
}

TEST_CASE( "non-empty move_only_function, when move constructed from, then the source becomes empty",
		   "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int()> source = []() { return 99; };

	// When
	mclo::move_only_function<int()> destination = std::move( source );

	// Then
	CHECK_FALSE( static_cast<bool>( source ) );
	REQUIRE( static_cast<bool>( destination ) );
	CHECK( destination() == 99 );
}

TEST_CASE( "non-empty move_only_function, when move assigned from, then it adopts the new target",
		   "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int()> source = []() { return 1; };
	mclo::move_only_function<int()> destination = []() { return 2; };

	// When
	destination = std::move( source );

	// Then
	CHECK_FALSE( static_cast<bool>( source ) );
	CHECK( destination() == 1 );
}

TEST_CASE( "non-empty move_only_function, when assigned nullptr, then it becomes empty", "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int()> function = []() { return 5; };

	// When
	function = nullptr;

	// Then
	CHECK_FALSE( static_cast<bool>( function ) );
}

TEST_CASE( "move_only_function constructed in place from arguments, when invoked, then uses the built target",
		   "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int( int )> function( std::in_place_type<multiplier>, 3 );

	// When / Then
	CHECK( function( 4 ) == 12 );
}

TEST_CASE( "move_only_function constructed in place from an initializer list, when invoked, then uses the built target",
		   "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int()> function( std::in_place_type<sum_list>, { 1, 2, 3, 4 } );

	// When / Then
	CHECK( function() == 10 );
}

TEST_CASE( "move_only_function with a const signature, when invoked through const access, then calls the target",
		   "[move_only_function]" )
{
	// Given
	const mclo::move_only_function<int( int ) const> function = multiplier{ 5 };

	// When / Then
	CHECK( function( 6 ) == 30 );
}

TEST_CASE( "move_only_function with an rvalue-ref signature, when invoked on an rvalue, then calls the target",
		   "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int() &&> function = []() { return 21; };

	// When / Then
	CHECK( std::move( function )() == 21 );
}

TEST_CASE( "move_only_function with a noexcept signature bound to a noexcept target, when invoked, then calls it",
		   "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int( int ) noexcept> function = []( const int value ) noexcept { return value * 2; };

	// When / Then
	CHECK( function( 8 ) == 16 );
	static_assert( std::is_nothrow_invocable_r_v<int, decltype( function )&, int> );
}

TEST_CASE( "two move_only_functions, when swapped, then they exchange targets", "[move_only_function]" )
{
	// Given
	mclo::move_only_function<int()> first = []() { return 1; };
	mclo::move_only_function<int()> second = []() { return 2; };

	// When
	swap( first, second );

	// Then
	CHECK( first() == 2 );
	CHECK( second() == 1 );
}

TEST_CASE( "move_only_function constructed from a null function pointer, when queried, then it is empty",
		   "[move_only_function]" )
{
	// Given
	int ( *pointer )( int ) = nullptr;
	const mclo::move_only_function<int( int )> function = pointer;

	// When / Then
	CHECK_FALSE( static_cast<bool>( function ) );
}

static_assert( !std::is_copy_constructible_v<mclo::move_only_function<int()>> );
static_assert( !std::is_copy_assignable_v<mclo::move_only_function<int()>> );
static_assert( std::is_nothrow_move_constructible_v<mclo::move_only_function<int()>> );
static_assert( std::is_nothrow_move_assignable_v<mclo::move_only_function<int()>> );
