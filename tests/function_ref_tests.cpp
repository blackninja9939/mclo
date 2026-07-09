#include <catch2/catch_test_macros.hpp>

#include "mclo/functional/function_ref.hpp"

#include <string>
#include <type_traits>

namespace
{
	int free_add( const int a, const int b ) noexcept
	{
		return a + b;
	}

	int free_negate( const int value )
	{
		return -value;
	}

	struct counter
	{
		int value = 0;

		int add( const int amount )
		{
			value += amount;
			return value;
		}

		int peek() const noexcept
		{
			return value;
		}
	};

	struct multiplier
	{
		int factor;

		int operator()( const int value ) const
		{
			return value * factor;
		}
	};
}

TEST_CASE( "function_ref referring to a lambda, when invoked, then forwards the call", "[function_ref]" )
{
	// Given
	int captured = 3;
	auto lambda = [ & ]( const int value ) { return value + captured; };
	const mclo::function_ref<int( int )> ref = lambda;

	// When / Then
	CHECK( ref( 4 ) == 7 );
}

TEST_CASE( "function_ref referring to a stateful callable, when the callable mutates, then it observes changes",
		   "[function_ref]" )
{
	// Given
	int total = 0;
	auto accumulate = [ & ]( const int value ) { total += value; };
	const mclo::function_ref<void( int )> ref = accumulate;

	// When
	ref( 5 );
	ref( 10 );

	// Then
	CHECK( total == 15 );
}

TEST_CASE( "function_ref referring to a function pointer, when invoked, then calls the function", "[function_ref]" )
{
	// Given
	const mclo::function_ref<int( int )> ref = &free_negate;

	// When / Then
	CHECK( ref( 8 ) == -8 );
}

TEST_CASE( "function_ref with a noexcept signature bound to a noexcept function, when invoked, then calls it",
		   "[function_ref]" )
{
	// Given
	const mclo::function_ref<int( int, int ) noexcept> ref = &free_add;

	// When / Then
	CHECK( ref( 2, 3 ) == 5 );
	static_assert( std::is_nothrow_invocable_r_v<int, decltype( ref ), int, int> );
}

TEST_CASE( "function_ref with a const signature bound to a const-callable object, when invoked, then calls it",
		   "[function_ref]" )
{
	// Given
	const multiplier times_three{ 3 };
	const mclo::function_ref<int( int ) const> ref = times_three;

	// When / Then
	CHECK( ref( 4 ) == 12 );
}

TEST_CASE( "function_ref bound to a member function via nontype and an object, when invoked, then calls the member",
		   "[function_ref]" )
{
	// Given
	counter object;
	const mclo::function_ref<int( int )> ref( mclo::nontype<&counter::add>, object );

	// When
	const int first = ref( 2 );
	const int second = ref( 3 );

	// Then
	CHECK( first == 2 );
	CHECK( second == 5 );
	CHECK( object.value == 5 );
}

TEST_CASE( "function_ref bound to a const member function via nontype and a pointer, when invoked, then calls it",
		   "[function_ref]" )
{
	// Given
	const counter object{ 42 };
	const mclo::function_ref<int() const> ref( mclo::nontype<&counter::peek>, &object );

	// When / Then
	CHECK( ref() == 42 );
}

TEST_CASE( "function_ref bound to a free function via nontype, when invoked, then calls the function",
		   "[function_ref]" )
{
	// Given
	const mclo::function_ref<int( int )> ref( mclo::nontype<&free_negate> );

	// When / Then
	CHECK( ref( 6 ) == -6 );
}

TEST_CASE( "function_ref referring to one callable, when copy-assigned from another, then it rebinds",
		   "[function_ref]" )
{
	// Given
	auto first = []( const int value ) { return value + 1; };
	auto second = []( const int value ) { return value + 100; };
	mclo::function_ref<int( int )> ref = first;
	const mclo::function_ref<int( int )> other = second;

	// When
	ref = other;

	// Then
	CHECK( ref( 1 ) == 101 );
}

TEST_CASE( "function_ref deduces its signature from a function pointer", "[function_ref]" )
{
	// Given
	mclo::function_ref ref = &free_add;

	// When / Then
	CHECK( ref( 4, 5 ) == 9 );
	static_assert( std::is_same_v<decltype( ref ), mclo::function_ref<int( int, int ) noexcept>> );
}

TEST_CASE( "function_ref deduces its signature from a pointer to member data bound to an object", "[function_ref]" )
{
	// Given
	counter object{ 42 };
	mclo::function_ref ref( mclo::nontype<&counter::value>, object );

	// When / Then
	CHECK( ref() == 42 );
	static_assert( std::is_same_v<decltype( ref ), mclo::function_ref<int&()>> );
}

static_assert( std::is_trivially_copyable_v<mclo::function_ref<int( int )>> );
static_assert( std::is_trivially_copyable_v<mclo::function_ref<void() const noexcept>> );
