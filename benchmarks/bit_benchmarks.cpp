#include <benchmark/benchmark.h>

#include "mclo/numeric/bit.hpp"

namespace
{
	void bit_reverse( benchmark::State& state )
	{
		std::uint32_t value = 0b00000000001010100000000000010101u;

		for ( auto _ : state )
		{
			auto result = mclo::bit_reverse( value );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( bit_reverse );

	template <std::unsigned_integral T>
	constexpr T bit_reverse_manual_loop( T value ) noexcept
	{
		T result = 0;
		for ( int i = 0; i < std::numeric_limits<T>::digits; ++i )
		{
			result <<= 1;
			result |= value & 1;
			value >>= 1;
		}
		return result;
	}

	void bit_reverse_manual( benchmark::State& state )
	{
		std::uint32_t value = 0b00000000001010100000000000010101u;

		for ( auto _ : state )
		{
			auto result = bit_reverse_manual_loop( value );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( bit_reverse_manual );

	void bit_compress( benchmark::State& state )
	{
		uint64_t x = 0xF0F0F0F0F0F0F0F0;
		uint64_t m = 0x00FF00FF00FF00FF;

		for ( auto _ : state )
		{
			auto result = mclo::bit_compress( x, m );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( bit_compress );

	void bit_compress_manual( benchmark::State& state )
	{
		uint64_t x = 0xF0F0F0F0F0F0F0F0;
		uint64_t m = 0x00FF00FF00FF00FF;

		for ( auto _ : state )
		{
			auto result = mclo::detail::bit_compress( x, m );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( bit_compress_manual );

	void bit_expand( benchmark::State& state )
	{
		uint64_t x = 0x00FF00FF00FF00FF;
		uint64_t m = 0xF0F0F0F0F0F0F0F0;

		for ( auto _ : state )
		{
			auto result = mclo::bit_expand( x, m );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( bit_expand );

	void bit_expand_manual( benchmark::State& state )
	{
		uint64_t x = 0x00FF00FF00FF00FF;
		uint64_t m = 0xF0F0F0F0F0F0F0F0;

		for ( auto _ : state )
		{
			auto result = mclo::detail::bit_expand( x, m );
			benchmark::DoNotOptimize( result );
		}
	}
	BENCHMARK( bit_expand_manual );
}
