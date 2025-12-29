#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/null_mutex.hpp"

#include <mutex>
#include <shared_mutex>

TEST_CASE( "null_mutex BasicLockable requirements", "[null_mutex]" )
{
	mclo::null_mutex mutex;
	const std::unique_lock lock( mutex );
}

TEST_CASE( "null_mutex Lockable requirements", "[null_mutex]" )
{
	mclo::null_mutex mutex;
	const std::unique_lock lock( mutex, std::try_to_lock );
}

TEST_CASE( "null_mutex TimedLockable requirements", "[null_mutex]" )
{
	using namespace std::chrono_literals;
	mclo::null_mutex mutex;
	SECTION( "For duration" )
	{
		const std::unique_lock lock( mutex, 2s );
	}
	SECTION( "Until timeout" )
	{
		const std::unique_lock lock( mutex, std::chrono::high_resolution_clock::now() + 2s );
	}
}

TEST_CASE( "null_mutex SharedLockable requirements", "[null_mutex]" )
{
	mclo::null_mutex mutex;
	SECTION( "lock_shared" )
	{
		const std::shared_lock lock( mutex );
	}
	SECTION( "try_lock_shared" )
	{
		const std::shared_lock lock( mutex, std::try_to_lock );
	}
}

TEST_CASE( "null_mutex SharedTimedLockable requirements", "[null_mutex]" )
{
	using namespace std::chrono_literals;
	mclo::null_mutex mutex;
	SECTION( "For duration" )
	{
		const std::shared_lock lock( mutex, 2s );
	}
	SECTION( "Until timeout" )
	{
		const std::shared_lock lock( mutex, std::chrono::high_resolution_clock::now() + 2s );
	}
}
