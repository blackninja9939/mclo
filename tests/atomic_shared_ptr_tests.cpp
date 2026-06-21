#include <catch2/catch_test_macros.hpp>

#include "mclo/threading/atomic_shared_ptr.hpp"
#include "mclo/threading/atomic_shared_ptr_ref_counter.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <random>
#include <thread>
#include <vector>

namespace
{
	std::atomic<int> g_live_nodes{ 0 };

	struct node : mclo::atomic_shared_ptr_ref_counter<node>
	{
		explicit node( const int value ) noexcept
			: m_value( value )
		{
			g_live_nodes.fetch_add( 1, std::memory_order_relaxed );
		}

		~node()
		{
			g_live_nodes.fetch_sub( 1, std::memory_order_relaxed );
		}

		int m_value;
	};

	[[nodiscard]] mclo::intrusive_ptr<node> make_node( const int value )
	{
		return mclo::intrusive_ptr<node>( new node( value ) );
	}
}

TEST_CASE( "atomic_shared_ptr default constructs empty", "[atomic_shared_ptr]" )
{
	const mclo::atomic_shared_ptr<node> ptr;
	CHECK( ptr.load() == nullptr );
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr is lock free", "[atomic_shared_ptr]" )
{
	const mclo::atomic_shared_ptr<node> ptr;
	CHECK( ptr.is_lock_free() );
	CHECK( mclo::atomic_shared_ptr<node>::is_always_lock_free );
}

TEST_CASE( "atomic_shared_ptr construct from intrusive_ptr", "[atomic_shared_ptr]" )
{
	{
		const mclo::intrusive_ptr<node> original = make_node( 42 );
		CHECK( original->use_count() == 1 );

		const mclo::atomic_shared_ptr<node> ptr{ original };
		CHECK( original->use_count() == 2 );

		const mclo::intrusive_ptr<node> loaded = ptr.load();
		REQUIRE( loaded );
		CHECK( loaded.get() == original.get() );
		CHECK( loaded->m_value == 42 );
		CHECK( original->use_count() == 3 );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr store releases previous value", "[atomic_shared_ptr]" )
{
	{
		mclo::atomic_shared_ptr<node> ptr{ make_node( 1 ) };
		CHECK( g_live_nodes.load() == 1 );

		ptr.store( make_node( 2 ) );
		CHECK( g_live_nodes.load() == 1 );

		const mclo::intrusive_ptr<node> loaded = ptr.load();
		REQUIRE( loaded );
		CHECK( loaded->m_value == 2 );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr exchange returns previous value", "[atomic_shared_ptr]" )
{
	{
		mclo::atomic_shared_ptr<node> ptr{ make_node( 10 ) };

		const mclo::intrusive_ptr<node> old = ptr.exchange( make_node( 20 ) );
		REQUIRE( old );
		CHECK( old->m_value == 10 );

		const mclo::intrusive_ptr<node> loaded = ptr.load();
		REQUIRE( loaded );
		CHECK( loaded->m_value == 20 );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr store nullptr clears", "[atomic_shared_ptr]" )
{
	{
		mclo::atomic_shared_ptr<node> ptr{ make_node( 5 ) };
		CHECK( g_live_nodes.load() == 1 );

		ptr = nullptr;
		CHECK( ptr.load() == nullptr );
		CHECK( g_live_nodes.load() == 0 );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr compare_exchange succeeds when expected matches", "[atomic_shared_ptr]" )
{
	{
		const mclo::intrusive_ptr<node> first = make_node( 1 );
		mclo::atomic_shared_ptr<node> ptr{ first };

		mclo::intrusive_ptr<node> expected = first;
		const mclo::intrusive_ptr<node> second = make_node( 2 );
		CHECK( ptr.compare_exchange_strong( expected, second ) );

		const mclo::intrusive_ptr<node> loaded = ptr.load();
		REQUIRE( loaded );
		CHECK( loaded.get() == second.get() );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr compare_exchange fails and reloads expected", "[atomic_shared_ptr]" )
{
	{
		const mclo::intrusive_ptr<node> actual = make_node( 1 );
		mclo::atomic_shared_ptr<node> ptr{ actual };

		mclo::intrusive_ptr<node> expected = make_node( 99 );
		const mclo::intrusive_ptr<node> desired = make_node( 2 );
		CHECK_FALSE( ptr.compare_exchange_strong( expected, desired ) );

		// On failure expected is updated to the actual stored value.
		CHECK( expected.get() == actual.get() );

		const mclo::intrusive_ptr<node> loaded = ptr.load();
		CHECK( loaded.get() == actual.get() );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr repeated store of same node keeps accounting", "[atomic_shared_ptr]" )
{
	{
		const mclo::intrusive_ptr<node> a = make_node( 1 );
		CHECK( a->use_count() == 1 );

		mclo::atomic_shared_ptr<node> ptr{ a };
		CHECK( a->use_count() == 2 );

		// Storing the same pointer over and over publishes, removes and re-publishes the identical control block,
		// the single threaded analogue of the ABA path. The internal/external accounting must net back to the slot
		// plus the local reference every time.
		for ( int i = 0; i < 100; ++i )
		{
			ptr.store( a );
			const mclo::intrusive_ptr<node> loaded = ptr.load();
			CHECK( loaded.get() == a.get() );
		}

		CHECK( a->use_count() == 2 );
		ptr = nullptr;
		CHECK( a->use_count() == 1 );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr compare_exchange to identical pointer succeeds", "[atomic_shared_ptr]" )
{
	{
		const mclo::intrusive_ptr<node> a = make_node( 7 );
		mclo::atomic_shared_ptr<node> ptr{ a };

		mclo::intrusive_ptr<node> expected = a;
		// Exercises the fast path where the desired pointer already matches the stored one.
		CHECK( ptr.compare_exchange_strong( expected, a ) );
		// a, expected and the slot all reference the node on success.
		CHECK( a->use_count() == 3 );
		expected = nullptr;
		CHECK( a->use_count() == 2 );

		const mclo::intrusive_ptr<node> loaded = ptr.load();
		CHECK( loaded.get() == a.get() );
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr concurrent load and store", "[atomic_shared_ptr]" )
{
	{
		mclo::atomic_shared_ptr<node> ptr{ make_node( 0 ) };

		constexpr int num_readers = 4;
		constexpr int num_writers = 2;
		constexpr int iterations = 20000;

		std::atomic<bool> start{ false };
		std::vector<std::thread> threads;

		for ( int r = 0; r < num_readers; ++r )
		{
			threads.emplace_back( [ &ptr, &start ] {
				while ( !start.load( std::memory_order_acquire ) )
				{
				}
				for ( int i = 0; i < iterations; ++i )
				{
					const mclo::intrusive_ptr<node> loaded = ptr.load();
					if ( loaded )
					{
						// Touch the object to let sanitizers catch any use-after-free.
						volatile int sink = loaded->m_value;
						( void )sink;
					}
				}
			} );
		}

		for ( int w = 0; w < num_writers; ++w )
		{
			threads.emplace_back( [ &ptr, &start ] {
				while ( !start.load( std::memory_order_acquire ) )
				{
				}
				for ( int i = 0; i < iterations; ++i )
				{
					ptr.store( make_node( i ) );
				}
			} );
		}

		start.store( true, std::memory_order_release );
		for ( auto& thread : threads )
		{
			thread.join();
		}
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr concurrent compare_exchange", "[atomic_shared_ptr]" )
{
	{
		mclo::atomic_shared_ptr<node> ptr{ make_node( 0 ) };

		constexpr int num_threads = 4;
		constexpr int iterations = 20000;

		std::atomic<bool> start{ false };
		std::vector<std::thread> threads;

		for ( int t = 0; t < num_threads; ++t )
		{
			threads.emplace_back( [ &ptr, &start, t ] {
				while ( !start.load( std::memory_order_acquire ) )
				{
				}
				for ( int i = 0; i < iterations; ++i )
				{
					mclo::intrusive_ptr<node> expected = ptr.load();
					mclo::intrusive_ptr<node> desired = make_node( t * iterations + i );
					ptr.compare_exchange_strong( expected, desired );
				}
			} );
		}

		start.store( true, std::memory_order_release );
		for ( auto& thread : threads )
		{
			thread.join();
		}
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr ABA recurring pointers do not corrupt counts", "[atomic_shared_ptr]" )
{
	{
		constexpr int pool_size = 4;
		std::array<mclo::intrusive_ptr<node>, pool_size> pool;
		for ( int i = 0; i < pool_size; ++i )
		{
			pool[ i ] = make_node( i );
		}
		REQUIRE( g_live_nodes.load() == pool_size );

		mclo::atomic_shared_ptr<node> ptr{ pool[ 0 ] };

		constexpr int num_storers = 3;
		constexpr int num_readers = 3;
		constexpr int iterations = 20000;

		std::atomic<bool> start{ false };
		std::vector<std::thread> threads;

		for ( int s = 0; s < num_storers; ++s )
		{
			threads.emplace_back( [ &ptr, &pool, &start, s ] {
				std::mt19937 rng( static_cast<std::uint32_t>( s + 1 ) );
				while ( !start.load( std::memory_order_acquire ) )
				{
				}
				for ( int i = 0; i < iterations; ++i )
				{
					// Drawing from a small fixed pool forces the same control block to be repeatedly removed from
					// and re-published into the slot, which is exactly the ABA window that the modular fold in
					// add_external_counters must reconcile.
					ptr.store( pool[ rng() % pool_size ] );
				}
			} );
		}

		for ( int r = 0; r < num_readers; ++r )
		{
			threads.emplace_back( [ &ptr, &start ] {
				while ( !start.load( std::memory_order_acquire ) )
				{
				}
				for ( int i = 0; i < iterations; ++i )
				{
					const mclo::intrusive_ptr<node> loaded = ptr.load();
					if ( loaded )
					{
						volatile int sink = loaded->m_value;
						( void )sink;
					}
				}
			} );
		}

		start.store( true, std::memory_order_release );
		for ( auto& thread : threads )
		{
			thread.join();
		}

		ptr = nullptr;

		// Every per-object counter must have unwound back to exactly the single reference held by the pool. A broken
		// modular fold would surface here as a stuck external counter (use_count != 1), a prematurely freed node
		// (fewer live nodes) or a leaked reference (more live nodes).
		CHECK( g_live_nodes.load() == pool_size );
		for ( const mclo::intrusive_ptr<node>& n : pool )
		{
			CHECK( n->use_count() == 1 );
		}
	}
	CHECK( g_live_nodes.load() == 0 );
}

TEST_CASE( "atomic_shared_ptr mixed concurrent operations", "[atomic_shared_ptr]" )
{
	{
		mclo::atomic_shared_ptr<node> ptr{ make_node( 0 ) };

		constexpr int num_threads = 6;
		constexpr int iterations = 20000;

		std::atomic<bool> start{ false };
		std::vector<std::thread> threads;

		for ( int t = 0; t < num_threads; ++t )
		{
			threads.emplace_back( [ &ptr, &start, t ] {
				std::mt19937 rng( static_cast<std::uint32_t>( t + 1 ) );
				while ( !start.load( std::memory_order_acquire ) )
				{
				}
				for ( int i = 0; i < iterations; ++i )
				{
					switch ( rng() % 4 )
					{
						case 0:
						{
							const mclo::intrusive_ptr<node> loaded = ptr.load();
							if ( loaded )
							{
								volatile int sink = loaded->m_value;
								( void )sink;
							}
							break;
						}
						case 1:
						{
							ptr.store( make_node( i ) );
							break;
						}
						case 2:
						{
							const mclo::intrusive_ptr<node> old = ptr.exchange( make_node( i ) );
							( void )old;
							break;
						}
						case 3:
						{
							mclo::intrusive_ptr<node> expected = ptr.load();
							ptr.compare_exchange_strong( expected, make_node( i ) );
							break;
						}
					}
				}
			} );
		}

		start.store( true, std::memory_order_release );
		for ( auto& thread : threads )
		{
			thread.join();
		}
	}
	CHECK( g_live_nodes.load() == 0 );
}
