#include <benchmark/benchmark.h>

#include "mclo/container/dense_slot_map.hpp"
#include "mclo/random/default_random_generator.hpp"

#include <unordered_map>

namespace
{
	void BM_IterateUnorderedMap( benchmark::State& state )
	{
		std::unordered_map<int, int> map;
		int next_key = 0;
		mclo::default_random_generator generator;
		while ( next_key < 100 )
		{
			map.emplace( next_key++, generator.uniform( 0, 100 ) );
		}

		for ( auto _ : state )
		{
			for ( auto& data : map )
			{
				benchmark::DoNotOptimize( data );
			}
		}
	}
	BENCHMARK( BM_IterateUnorderedMap );

	void BM_IterateDenseSlotMap( benchmark::State& state )
	{
		mclo::dense_slot_map<int> map;
		mclo::default_random_generator generator;
		for ( int i = 0; i < 100; ++i )
		{
			( void )map.insert( generator.uniform( 0, 100 ) );
		}

		for ( auto _ : state )
		{
			for ( auto& data : map )
			{
				benchmark::DoNotOptimize( data );
			}
		}
	}
	BENCHMARK( BM_IterateDenseSlotMap );
}
