#include <benchmark/benchmark.h>

#include "mclo/enum_range.hpp"

namespace
{
	enum class test_enum
	{
		first,
		second,
		third,
		fourth,
		fifth,
		enum_size
	};

	void BM_EnumRange( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			for ( const test_enum e : mclo::enum_range<test_enum>() )
			{
				benchmark::DoNotOptimize( e );
			}
		}
	}
	BENCHMARK( BM_EnumRange );

	void BM_EnumIndexLoop( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			using underlying_t = std::underlying_type_t<test_enum>;
			for ( underlying_t index = 0; index < static_cast<underlying_t>( test_enum::enum_size ); ++index )
			{
				test_enum e = static_cast<test_enum>( index );
				benchmark::DoNotOptimize( e );
			}
		}
	}
	BENCHMARK( BM_EnumIndexLoop );
}
