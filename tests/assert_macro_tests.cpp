#include "assert_macros.hpp"

#include "mclo/debug/assert.hpp"

TEST_CASE( "DEBUG_ASSERT caught", "[test_infra]" )
{
	auto func = []() { DEBUG_ASSERT( false, "Test debug assert run" ); };
	CHECK_ASSERTS( func(), "Test debug assert run" );
	REQUIRE_ASSERTS( func(), "Test debug assert run" );
}

TEST_CASE( "ASSERT caught", "[test_infra]" )
{
	auto func = []() { ASSERT( false, "Test assert run" ); };
	CHECK_ASSERTS( func(), "Test assert run" );
	REQUIRE_ASSERTS( func(), "Test assert run" );
}

TEST_CASE( "ASSUME caught", "[test_infra]" )
{
	auto func = []() { ASSUME( false, "Test assume run" ); };
	CHECK_ASSERTS( func(), "Test assume run" );
	REQUIRE_ASSERTS( func(), "Test assume run" );
}

TEST_CASE( "PANIC caught", "[test_infra]" )
{
	auto func = []() { PANIC( "Test panic run" ); };
	CHECK_ASSERTS( func(), "Test panic run" );
	REQUIRE_ASSERTS( func(), "Test panic run" );
}

TEST_CASE( "UNREACHABLE caught", "[test_infra]" )
{
	auto func = []() { UNREACHABLE( "Test unreachable run" ); };
	CHECK_ASSERTS( func(), "Test unreachable run" );
	REQUIRE_ASSERTS( func(), "Test unreachable run" );
}

TEST_CASE( "DEBUG_ASSERT_VAL caught", "[test_infra]" )
{
	auto func = []() { [[maybe_unused]] const bool b = DEBUG_ASSERT_VAL( false, "Test debug assert run" ); };
	CHECK_ASSERTS( func(), "Test debug assert run" );
	REQUIRE_ASSERTS( func(), "Test debug assert run" );
}

TEST_CASE( "ASSERT_VAL caught", "[test_infra]" )
{
	auto func = []() { [[maybe_unused]] const bool b = ASSERT_VAL( false, "Test assert run" ); };
	CHECK_ASSERTS( func(), "Test assert run" );
	REQUIRE_ASSERTS( func(), "Test assert run" );
}

TEST_CASE( "ASSUME_VAL caught", "[test_infra]" )
{
	auto func = []() { [[maybe_unused]] const bool b = ASSUME_VAL( false, "Test assume run" ); };
	CHECK_ASSERTS( func(), "Test assume run" );
	REQUIRE_ASSERTS( func(), "Test assume run" );
}
