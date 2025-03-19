#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/atomic_shared_ptr.hpp"

TEST_CASE( "Default atomic shared_ptr is empty", "[atomic_shared_ptr]" )
{
	mclo::atomic_shared_ptr<int> ptr;
	CHECK( ptr.load() == nullptr );
}

TEST_CASE( "Atomic shared_ptr can be constructed with nullptr", "[atomic_shared_ptr]" )
{
	mclo::atomic_shared_ptr<int> ptr( nullptr );
	CHECK( ptr.load() == nullptr );
}

TEST_CASE( "Atomic shared_ptr can be assigned nullptr", "[atomic_shared_ptr]" )
{
	using atomic_ptr = mclo::atomic_shared_ptr<int>;
	atomic_ptr ptr( atomic_ptr::shared_ptr( new int( 42 ) ) );
	ptr = nullptr;
	CHECK( ptr.load() == nullptr );
}

TEST_CASE( "Atomic shared_ptr store new value", "[atomic_shared_ptr]" )
{
	using atomic_ptr = mclo::atomic_shared_ptr<int>;
	atomic_ptr ptr( atomic_ptr::shared_ptr( new int( 42 ) ) );
	ptr.store( atomic_ptr::shared_ptr( new int( 11 ) ) );
	CHECK( ptr.load() == nullptr );
}
