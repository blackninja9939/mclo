#include <catch2/catch_test_macros.hpp>

#include "mclo/allocator/arena_allocator.hpp"

TEST_CASE( "MemoryArena_ResetOnFreshlyConstructed_IsSafe", "[arena_allocator]" )
{
	mclo::memory_arena arena( 64 );

	arena.reset();

	void* const ptr = arena.allocate( 16 );
	CHECK( ptr );
}

TEST_CASE( "MemoryArena_ResetConsolidateOnFreshlyConstructed_IsSafe", "[arena_allocator]" )
{
	mclo::memory_arena arena( 64 );

	arena.reset_consolidate();

	void* const ptr = arena.allocate( 16 );
	CHECK( ptr );
}

TEST_CASE( "MemoryArena_ResetAfterAllocations_ReusesChunks", "[arena_allocator]" )
{
	mclo::memory_arena arena( 64 );
	void* const first = arena.allocate( 16 );

	arena.reset();

	// Reset reuses the existing chunk so the first allocation returns the same address
	void* const second = arena.allocate( 16 );
	CHECK( first == second );
}

TEST_CASE( "MemoryArena_ResetConsolidateAfterGrowth_ReturnsSingleUsableChunk", "[arena_allocator]" )
{
	mclo::memory_arena arena( 16 );
	arena.allocate( 16 );
	arena.allocate( 64 ); // Forces a second, larger chunk to be linked in

	arena.reset_consolidate();

	void* const ptr = arena.allocate( 64 );
	CHECK( ptr );
}
