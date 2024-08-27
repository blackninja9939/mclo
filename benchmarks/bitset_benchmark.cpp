#include <benchmark/benchmark.h>

#include "mclo/bitset.hpp"

#include <bitset>

namespace
{
	template <std::size_t Bits, std::unsigned_integral UnderlyingType = std::size_t>
	class dynamic_bitset_wrapper : public mclo::dynamic_bitset<UnderlyingType>
	{
	public:
		dynamic_bitset_wrapper()
			: mclo::dynamic_bitset<UnderlyingType>( Bits )
		{
		}
	};

#define BENCHMARK_STD_BITSET( FUNC, SETUP )                                                                            \
	BENCHMARK( FUNC<std::bitset<72>> )->Apply( &SETUP::large );                                                        \
	BENCHMARK( FUNC<std::bitset<16>> )->Apply( &SETUP::small )

#define BENCHMARK_BITSET( FUNC, SETUP )                                                                                \
	BENCHMARK( FUNC<mclo::bitset<72>> )->Apply( &SETUP::large );                                                       \
	BENCHMARK( FUNC<dynamic_bitset_wrapper<72>> )->Apply( &SETUP::large );                                             \
	BENCHMARK( FUNC<mclo::bitset<16>> )->Apply( &SETUP::small );                                                       \
	BENCHMARK( FUNC<dynamic_bitset_wrapper<16>> )->Apply( &SETUP::small )

	struct find_first_setup
	{
		static void small( benchmark::internal::Benchmark* const b )
		{
			b->Arg( 8 );
		}
		static void large( benchmark::internal::Benchmark* const b )
		{
			b->Arg( 8 )->Arg( 63 );
		}
	};

	template <typename BitSet>
	void Bitset_FindFirstSet( benchmark::State& state )
	{
		BitSet set;
		set.set( state.range() );
		for ( auto _ : state )
		{
			std::size_t pos = set.find_first_set();
			benchmark::DoNotOptimize( pos );
		}
	}
	BENCHMARK_BITSET( Bitset_FindFirstSet, find_first_setup );

	template <typename BitSet>
	void Bitset_StdFindFirstSet( benchmark::State& state )
	{
		BitSet set;
		set.set( state.range() );
		for ( auto _ : state )
		{
			std::size_t i = 0;
			for ( ; i < set.size(); ++i )
			{
				if ( set.test( i ) )
				{
					break;
				}
			}
			benchmark::DoNotOptimize( i );
		}
	}
	BENCHMARK_STD_BITSET( Bitset_StdFindFirstSet, find_first_setup );

	template <typename BitSet>
	void Bitset_FindFirstUnset( benchmark::State& state )
	{
		BitSet set;
		set.set().reset( state.range() );
		for ( auto _ : state )
		{
			std::size_t pos = set.find_first_unset();
			benchmark::DoNotOptimize( pos );
		}
	}
	BENCHMARK_BITSET( Bitset_FindFirstUnset, find_first_setup );

	template <typename BitSet>
	void Bitset_StdFindFirstUnset( benchmark::State& state )
	{
		BitSet set;
		set.set().reset( state.range() );
		for ( auto _ : state )
		{
			std::size_t i = 0;
			for ( ; i < set.size(); ++i )
			{
				if ( !set.test( i ) )
				{
					break;
				}
			}
			benchmark::DoNotOptimize( i );
		}
	}
	BENCHMARK_STD_BITSET( Bitset_StdFindFirstUnset, find_first_setup );

	struct for_each_set_setup
	{
		static void small( benchmark::internal::Benchmark* const b )
		{
			b->Args( { 3, 2, 7, 8 } );
		}
		static void large( benchmark::internal::Benchmark* const b )
		{
			small( b );
			b->Args( { 5, 4, 19, 38, 55, 68 } );
		}
	};

	template <typename BitSet>
	void Bitset_ForEachSet( benchmark::State& state )
	{
		BitSet set;
		const auto num_args = state.range( 0 );
		for ( int i = 1; i <= num_args; i++ )
		{
			set.set( state.range( i ) );
		}

		for ( auto _ : state )
		{
			std::size_t sum = 0;
			set.for_each_set( [ &sum ]( const std::size_t i ) { sum += i; } );
			benchmark::DoNotOptimize( sum );
		}
	}
	BENCHMARK_BITSET( Bitset_ForEachSet, for_each_set_setup );

	template <typename BitSet>
	void Bitset_StdForEachSet( benchmark::State& state )
	{
		BitSet set;
		const auto num_args = state.range( 0 );
		for ( int i = 1; i <= num_args; i++ )
		{
			set.set( state.range( i ) );
		}

		for ( auto _ : state )
		{
			std::size_t sum = 0;
			for ( std::size_t i = 0; i < set.size(); ++i )
			{
				if ( set.test( i ) )
				{
					sum += i;
				}
			}
			benchmark::DoNotOptimize( sum );
		}
	}
	BENCHMARK_STD_BITSET( Bitset_StdForEachSet, for_each_set_setup );
}
