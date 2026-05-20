#include <benchmark/benchmark.h>

#include "mclo/container/packed_int_array.hpp"

namespace
{
	// --- Fill vs set in a loop ---

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_fill( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;
		using value_type = typename arr::value_type;
		static constexpr value_type fill_value = arr::max_value / 2;

		arr a;
		for ( auto _ : state )
		{
			a.fill( fill_value );
			benchmark::DoNotOptimize( a.underlying().data() );
			benchmark::ClobberMemory();
		}
	}

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_set_loop( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;
		using value_type = typename arr::value_type;
		static constexpr value_type fill_value = arr::max_value / 2;

		arr a;
		for ( auto _ : state )
		{
			for ( std::size_t i = 0; i < Size; ++i )
			{
				a.set( i, fill_value );
			}
			benchmark::DoNotOptimize( a.underlying().data() );
			benchmark::ClobberMemory();
		}
	}

	BENCHMARK( packed_int_array_fill<7, 8, std::uint8_t> );
	BENCHMARK( packed_int_array_fill<7, 64, std::uint8_t> );
	BENCHMARK( packed_int_array_fill<7, 512, std::uint8_t> );
	BENCHMARK( packed_int_array_fill<7, 4096, std::uint8_t> );
	BENCHMARK( packed_int_array_fill<7, 8192, std::uint8_t> );
	BENCHMARK( packed_int_array_fill<7, 8, std::uint32_t> );
	BENCHMARK( packed_int_array_fill<7, 64, std::uint32_t> );
	BENCHMARK( packed_int_array_fill<7, 512, std::uint32_t> );
	BENCHMARK( packed_int_array_fill<7, 4096, std::uint32_t> );
	BENCHMARK( packed_int_array_fill<7, 8192, std::uint32_t> );
	BENCHMARK( packed_int_array_fill<7, 8, std::uint64_t> );
	BENCHMARK( packed_int_array_fill<7, 64, std::uint64_t> );
	BENCHMARK( packed_int_array_fill<7, 512, std::uint64_t> );
	BENCHMARK( packed_int_array_fill<7, 4096, std::uint64_t> );
	BENCHMARK( packed_int_array_fill<7, 8192, std::uint64_t> );

	BENCHMARK( packed_int_array_set_loop<7, 8, std::uint8_t> );
	BENCHMARK( packed_int_array_set_loop<7, 64, std::uint8_t> );
	BENCHMARK( packed_int_array_set_loop<7, 512, std::uint8_t> );
	BENCHMARK( packed_int_array_set_loop<7, 4096, std::uint8_t> );
	BENCHMARK( packed_int_array_set_loop<7, 8192, std::uint8_t> );
	BENCHMARK( packed_int_array_set_loop<7, 8, std::uint32_t> );
	BENCHMARK( packed_int_array_set_loop<7, 64, std::uint32_t> );
	BENCHMARK( packed_int_array_set_loop<7, 512, std::uint32_t> );
	BENCHMARK( packed_int_array_set_loop<7, 4096, std::uint32_t> );
	BENCHMARK( packed_int_array_set_loop<7, 8192, std::uint32_t> );
	BENCHMARK( packed_int_array_set_loop<7, 8, std::uint64_t> );
	BENCHMARK( packed_int_array_set_loop<7, 64, std::uint64_t> );
	BENCHMARK( packed_int_array_set_loop<7, 512, std::uint64_t> );
	BENCHMARK( packed_int_array_set_loop<7, 4096, std::uint64_t> );
	BENCHMARK( packed_int_array_set_loop<7, 8192, std::uint64_t> );

	// --- Get + set vs exchange ---

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_get_then_set( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;
		using value_type = typename arr::value_type;

		arr a( arr::max_value / 2 );
		for ( auto _ : state )
		{
			for ( std::size_t i = 0; i < Size; ++i )
			{
				auto old = a.get( i );
				a.set( i, static_cast<value_type>( old ^ value_type{ 1 } ) );
				benchmark::DoNotOptimize( old );
			}
		}
	}

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_exchange( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;
		using value_type = typename arr::value_type;
		static constexpr value_type new_value = arr::max_value / 3;

		arr a( arr::max_value / 2 );
		for ( auto _ : state )
		{
			for ( std::size_t i = 0; i < Size; ++i )
			{
				auto old = a.exchange( i, new_value );
				benchmark::DoNotOptimize( old );
			}
		}
	}

	BENCHMARK( packed_int_array_get_then_set<7, 8, std::uint32_t> );
	BENCHMARK( packed_int_array_get_then_set<7, 64, std::uint32_t> );
	BENCHMARK( packed_int_array_get_then_set<7, 512, std::uint32_t> );
	BENCHMARK( packed_int_array_get_then_set<7, 4096, std::uint32_t> );
	BENCHMARK( packed_int_array_get_then_set<7, 8192, std::uint32_t> );

	BENCHMARK( packed_int_array_exchange<7, 8, std::uint32_t> );
	BENCHMARK( packed_int_array_exchange<7, 64, std::uint32_t> );
	BENCHMARK( packed_int_array_exchange<7, 512, std::uint32_t> );
	BENCHMARK( packed_int_array_exchange<7, 4096, std::uint32_t> );
	BENCHMARK( packed_int_array_exchange<7, 8192, std::uint32_t> );

	// --- Get: byte-offset load path vs standard path, with and without boundary crossing ---
	//
	// <8, uint32_t>:  byte_offset=true,  crossing=false
	// <13, uint32_t>: byte_offset=true,  crossing=true
	// <32, uint32_t>: byte_offset=false, crossing=false
	// <27, uint32_t>: byte_offset=false, crossing=true

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_get( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;

		arr a( arr::max_value / 2 );
		for ( auto _ : state )
		{
			for ( std::size_t i = 0; i < Size; ++i )
			{
				auto val = a.get( i );
				benchmark::DoNotOptimize( val );
			}
		}
	}

	BENCHMARK( packed_int_array_get<8, 8192, std::uint32_t> );
	BENCHMARK( packed_int_array_get<13, 8192, std::uint32_t> );
	BENCHMARK( packed_int_array_get<32, 8192, std::uint32_t> );
	BENCHMARK( packed_int_array_get<27, 8192, std::uint32_t> );

	// --- for_each vs get in a loop ---

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_for_each( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;
		using value_type = typename arr::value_type;

		arr a( arr::max_value / 2 );
		for ( auto _ : state )
		{
			value_type sum{};
			a.for_each( [ &sum ]( const value_type val ) { sum += val; } );
			benchmark::DoNotOptimize( sum );
		}
	}

	template <std::size_t BitWidth, std::size_t Size, typename UnderlyingType>
	void packed_int_array_get_loop( benchmark::State& state )
	{
		using arr = mclo::packed_int_array<BitWidth, Size, UnderlyingType>;
		using value_type = typename arr::value_type;

		arr a( arr::max_value / 2 );
		for ( auto _ : state )
		{
			value_type sum{};
			for ( std::size_t i = 0; i < Size; ++i )
			{
				sum += a.get( i );
			}
			benchmark::DoNotOptimize( sum );
		}
	}

	// Crossing + byte-offset load
	BENCHMARK( packed_int_array_for_each<7, 8, std::size_t> );
	BENCHMARK( packed_int_array_for_each<7, 512, std::size_t> );
	BENCHMARK( packed_int_array_for_each<7, 8192, std::size_t> );
	BENCHMARK( packed_int_array_get_loop<7, 8, std::size_t> );
	BENCHMARK( packed_int_array_get_loop<7, 512, std::size_t> );
	BENCHMARK( packed_int_array_get_loop<7, 8192, std::size_t> );

	// Non-crossing: for_each can extract N values per physical load
	BENCHMARK( packed_int_array_for_each<8, 8, std::size_t> );
	BENCHMARK( packed_int_array_for_each<8, 512, std::size_t> );
	BENCHMARK( packed_int_array_for_each<8, 8192, std::size_t> );
	BENCHMARK( packed_int_array_get_loop<8, 8, std::size_t> );
	BENCHMARK( packed_int_array_get_loop<8, 512, std::size_t> );
	BENCHMARK( packed_int_array_get_loop<8, 8192, std::size_t> );

	// Crossing, no byte-offset load: fallback delegates to get()
	BENCHMARK( packed_int_array_for_each<27, 8, std::uint32_t> );
	BENCHMARK( packed_int_array_for_each<27, 512, std::uint32_t> );
	BENCHMARK( packed_int_array_for_each<27, 8192, std::uint32_t> );
	BENCHMARK( packed_int_array_get_loop<27, 8, std::uint32_t> );
	BENCHMARK( packed_int_array_get_loop<27, 512, std::uint32_t> );
	BENCHMARK( packed_int_array_get_loop<27, 8192, std::uint32_t> );
}
