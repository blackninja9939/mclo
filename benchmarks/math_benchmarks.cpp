#include <benchmark/benchmark.h>

#include "mclo/numeric/log2.hpp"
#include "mclo/numeric/math.hpp"

namespace
{
	void log2_floor( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::uint64_t value = mclo::log2_floor( static_cast<std::uint64_t>( state.range() ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( log2_floor )->Arg( 32 );

	template <std::unsigned_integral T>
	constexpr T log2_floor_manual_loop( T value ) noexcept
	{
		if ( value <= 1 )
		{
			return 1;
		}

		T power = 0;
		while ( value >>= T( 0x1 ) )
		{
			++power;
		}
		return power;
	}

	void log2_floor_manual( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::uint64_t value = log2_floor_manual_loop( static_cast<std::uint64_t>( state.range() ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( log2_floor_manual )->Arg( 32 );

	void log2_floor_std( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto value =
				static_cast<std::uint64_t>( std::floor( std::log2( static_cast<std::uint64_t>( state.range() ) ) ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( log2_floor_std )->Arg( 32 );

	void log2_ceil( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::uint64_t value = mclo::log2_ceil( static_cast<std::uint64_t>( state.range() ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( log2_ceil )->Arg( 32 );

	template <std::unsigned_integral T>
	constexpr T log2_ceil_manual_loop( T value ) noexcept
	{
		if ( value <= 1 )
		{
			return 1;
		}

		T power = 0;
		T tmp = value - 1; // ceil bumps up if not pow2
		while ( tmp >>= T( 0x1 ) )
		{
			++power;
		}
		return power + 1;
	}

	void log2_ceil_manual( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			std::uint64_t value = log2_ceil_manual_loop( static_cast<std::uint64_t>( state.range() ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( log2_ceil_manual )->Arg( 32 );

	void log2_ceil_std( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto value =
				static_cast<std::uint64_t>( std::ceil( std::log2( static_cast<std::uint64_t>( state.range() ) ) ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( log2_ceil_std )->Arg( 32 );

	void int_ceil_div_signed( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto value = mclo::ceil_divide( state.range( 0 ), state.range( 1 ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( int_ceil_div_signed )->Args( { 6, -4 } );

	void int_ceil_div_unsigned( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto value = mclo::ceil_divide( static_cast<std::uint64_t>( state.range( 0 ) ),
											static_cast<std::uint64_t>( state.range( 1 ) ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( int_ceil_div_unsigned )->Args( { 6, 4 } );

	template <std::integral T>
	[[nodiscard]] constexpr T ceil_divide_old( const T dividend, const T divisor ) MCLO_NOEXCEPT_TESTS
	{
		if ( dividend > 0 == divisor > 0 )
		{
			return ( dividend + divisor - 1 ) / divisor;
		}
		else
		{
			return dividend / divisor;
		}
	}

	void int_ceil_div_signed_old( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto value = ceil_divide_old( state.range( 0 ), state.range( 1 ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( int_ceil_div_signed_old )->Args( { 6, -4 } );

	void int_ceil_div_unsigned_old( benchmark::State& state )
	{
		for ( auto _ : state )
		{
			auto value = ceil_divide_old( static_cast<std::uint64_t>( state.range( 0 ) ),
										  static_cast<std::uint64_t>( state.range( 1 ) ) );
			benchmark::DoNotOptimize( value );
		}
	}
	BENCHMARK( int_ceil_div_unsigned_old )->Args( { 6, 4 } );
}
