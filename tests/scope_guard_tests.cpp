#include <catch2/catch_test_macros.hpp>

#include "mclo/utility/scope_guard.hpp"

#include <stdexcept>

// --- scope_exit ---

TEST_CASE( "scope_exit, normal exit, calls function", "[utility][scope_guard]" )
{
	bool called = false;

	{
		mclo::scope_exit guard( [&]() { called = true; } );
		CHECK_FALSE( called );
	}

	CHECK( called );
}

TEST_CASE( "scope_exit, exception exit, calls function", "[utility][scope_guard]" )
{
	bool called = false;

	try
	{
		mclo::scope_exit guard( [&]() { called = true; } );
		throw std::runtime_error( "test" );
	}
	catch ( ... )
	{
	}

	CHECK( called );
}

TEST_CASE( "scope_exit, multiple guards, calls in reverse order", "[utility][scope_guard]" )
{
	int order = 0;
	int first_called_at = 0;
	int second_called_at = 0;

	{
		mclo::scope_exit guard1( [&]() { first_called_at = ++order; } );
		mclo::scope_exit guard2( [&]() { second_called_at = ++order; } );
	}

	CHECK( second_called_at == 1 );
	CHECK( first_called_at == 2 );
}

TEST_CASE( "scope_exit, constexpr, calls function", "[utility][scope_guard]" )
{
	constexpr int result = [] {
		int value = 0;
		{
			mclo::scope_exit guard( [&]() { value = 42; } );
		}
		return value;
	}();

	STATIC_CHECK( result == 42 );
}

// --- scope_fail ---

TEST_CASE( "scope_fail, normal exit, does not call function", "[utility][scope_guard]" )
{
	bool called = false;

	{
		mclo::scope_fail guard( [&]() { called = true; } );
	}

	CHECK_FALSE( called );
}

TEST_CASE( "scope_fail, exception exit, calls function", "[utility][scope_guard]" )
{
	bool called = false;

	try
	{
		mclo::scope_fail guard( [&]() { called = true; } );
		throw std::runtime_error( "test" );
	}
	catch ( ... )
	{
	}

	CHECK( called );
}

TEST_CASE( "scope_fail, nested exception then normal inner exit, does not call inner guard", "[utility][scope_guard]" )
{
	bool outer_called = false;
	bool inner_called = false;

	try
	{
		mclo::scope_fail outer_guard( [&]() { outer_called = true; } );

		try
		{
			mclo::scope_fail inner_guard( [&]() { inner_called = true; } );
			// Inner scope exits normally
		}
		catch ( ... )
		{
		}

		throw std::runtime_error( "test" );
	}
	catch ( ... )
	{
	}

	CHECK( outer_called );
	CHECK_FALSE( inner_called );
}

// --- scope_success ---

TEST_CASE( "scope_success, normal exit, calls function", "[utility][scope_guard]" )
{
	bool called = false;

	{
		mclo::scope_success guard( [&]() { called = true; } );
	}

	CHECK( called );
}

TEST_CASE( "scope_success, exception exit, does not call function", "[utility][scope_guard]" )
{
	bool called = false;

	try
	{
		mclo::scope_success guard( [&]() { called = true; } );
		throw std::runtime_error( "test" );
	}
	catch ( ... )
	{
	}

	CHECK_FALSE( called );
}
