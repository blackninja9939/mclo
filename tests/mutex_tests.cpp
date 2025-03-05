#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/mutex.hpp"

TEST_CASE( "mutex lock", "[mutex]" )
{
	mclo::mutex mutex;
	mutex.lock();
	mutex.unlock();
}

TEST_CASE( "mutex try_lock", "[mutex]" )
{
	mclo::mutex mutex;
	const bool result = mutex.try_lock();
	CHECK( result );
	mutex.unlock();
}

TEST_CASE( "mutex try_lock fail", "[mutex]" )
{
	mclo::mutex mutex;
	mutex.lock();
	CHECK_FALSE( mutex.try_lock() );
	mutex.unlock();
}

TEST_CASE( "mutex RAII lock", "[mutex]" )
{
	mclo::mutex mutex;
	const std::unique_lock lock( mutex );
	CHECK( lock.owns_lock() );
}
