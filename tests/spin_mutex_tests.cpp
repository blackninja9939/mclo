#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/spin_mutex.hpp"

#include <mutex>

TEST_CASE( "spin_mutex BasicLockable requirements", "[spin_mutex]" )
{
	mclo::spin_mutex mutex;
	const std::unique_lock lock( mutex );
}

TEST_CASE( "spin_mutex Lockable requirements", "[spin_mutex]" )
{
	mclo::spin_mutex mutex;
	const std::unique_lock lock( mutex, std::try_to_lock );
}
