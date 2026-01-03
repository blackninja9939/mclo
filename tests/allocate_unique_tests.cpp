#include <catch2/catch_test_macros.hpp>

#include "mclo/memory/allocate_unique.hpp"

namespace
{
	enum class track_state : std::uint8_t
	{
		none,
		constructed,
		destroyed,
	};

	struct tracker
	{
		track_state& state;

		explicit tracker( track_state& s ) noexcept
			: state( s )
		{
			state = track_state::constructed;
		}

		~tracker() noexcept
		{
			state = track_state::destroyed;
		}
	};

	static_assert( sizeof( std::unique_ptr<tracker> ) ==
					   sizeof( std::unique_ptr<tracker, mclo::allocation_deleter<std::allocator<tracker>>> ),
				   "For an empty allocator there should be no overhead to the unique_ptr" );
}

TEST_CASE( "allocator_new allocates type", "[memory]" )
{
	track_state state = track_state::none;
	std::allocator<tracker> allocator;
	auto ptr = mclo::allocator_new<tracker>( allocator, state );
	REQUIRE( ptr != nullptr );
	CHECK( state == track_state::constructed );
	mclo::allocator_delete( allocator, ptr );
	CHECK( state == track_state::destroyed );
}

TEST_CASE( "allocate_unique allocates and frees type", "[memory]" )
{
	track_state state = track_state::none;
	{
		std::allocator<tracker> allocator;
		auto ptr = mclo::allocate_unique<tracker>( allocator, state );
		REQUIRE( ptr != nullptr );
		CHECK( state == track_state::constructed );
	}
	CHECK( state == track_state::destroyed );
}
