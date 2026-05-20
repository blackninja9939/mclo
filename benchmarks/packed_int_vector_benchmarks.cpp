#include <benchmark/benchmark.h>

#include "mclo/container/packed_int_vector.hpp"

namespace
{
	void packed_int_vector_size_args( benchmark::internal::Benchmark* b )
	{
		b->RangeMultiplier( 2 )->Range( 8, 8192 );
	}

	// --- Fill vs set in a loop ---

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_fill( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;
		using value_type = typename vec::value_type;
		static constexpr value_type fill_value = vec::max_value / 2;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count );
		for ( auto _ : state )
		{
			v.fill( fill_value );
			benchmark::DoNotOptimize( v.underlying().data() );
			benchmark::ClobberMemory();
		}
	}

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_set_loop( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;
		using value_type = typename vec::value_type;
		static constexpr value_type fill_value = vec::max_value / 2;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count );
		for ( auto _ : state )
		{
			for ( typename vec::size_type i = 0; i < count; ++i )
			{
				v.set( i, fill_value );
			}
			benchmark::DoNotOptimize( v.underlying().data() );
			benchmark::ClobberMemory();
		}
	}

	BENCHMARK( packed_int_vector_fill<7, std::uint8_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_fill<7, std::uint16_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_fill<7, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_fill<7, std::uint64_t> )->Apply( packed_int_vector_size_args );

	BENCHMARK( packed_int_vector_set_loop<7, std::uint8_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_set_loop<7, std::uint16_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_set_loop<7, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_set_loop<7, std::uint64_t> )->Apply( packed_int_vector_size_args );

	// --- Get + set vs exchange ---

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_get_then_set( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;
		using value_type = typename vec::value_type;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count, vec::max_value / 2 );
		for ( auto _ : state )
		{
			for ( typename vec::size_type i = 0; i < count; ++i )
			{
				auto old = v.get( i );
				v.set( i, static_cast<value_type>( old ^ value_type{ 1 } ) );
				benchmark::DoNotOptimize( old );
			}
		}
	}

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_exchange( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;
		using value_type = typename vec::value_type;
		static constexpr value_type new_value = vec::max_value / 3;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count, vec::max_value / 2 );
		for ( auto _ : state )
		{
			for ( typename vec::size_type i = 0; i < count; ++i )
			{
				auto old = v.exchange( i, new_value );
				benchmark::DoNotOptimize( old );
			}
		}
	}

	BENCHMARK( packed_int_vector_get_then_set<7, std::uint8_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get_then_set<7, std::uint16_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get_then_set<7, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get_then_set<7, std::uint64_t> )->Apply( packed_int_vector_size_args );

	BENCHMARK( packed_int_vector_exchange<7, std::uint8_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_exchange<7, std::uint16_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_exchange<7, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_exchange<7, std::uint64_t> )->Apply( packed_int_vector_size_args );

	// --- Get: byte-offset load path vs standard path, with and without boundary crossing ---
	//
	// use_byte_offset_load = (BitWidth + 7 <= bits_per_underlying) on little-endian
	//
	// <8, uint32_t>:  byte_offset=true,  crossing=false (32 % 8 == 0)
	// <13, uint32_t>: byte_offset=true,  crossing=true  (32 % 13 != 0)
	// <32, uint32_t>: byte_offset=false, crossing=false (32 % 32 == 0)
	// <27, uint32_t>: byte_offset=false, crossing=true  (32 % 27 != 0)

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_get( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count, vec::max_value / 2 );
		for ( auto _ : state )
		{
			for ( typename vec::size_type i = 0; i < count; ++i )
			{
				auto val = v.get( i );
				benchmark::DoNotOptimize( val );
			}
		}
	}

	BENCHMARK( packed_int_vector_get<8, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get<13, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get<32, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get<27, std::uint32_t> )->Apply( packed_int_vector_size_args );

	// --- for_each vs get in a loop ---

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_for_each( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;
		using value_type = typename vec::value_type;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count, vec::max_value / 2 );
		for ( auto _ : state )
		{
			value_type sum{};
			v.for_each( [ &sum ]( const value_type val ) { sum += val; } );
			benchmark::DoNotOptimize( sum );
		}
	}

	template <std::size_t BitWidth, typename UnderlyingType>
	void packed_int_vector_get_loop( benchmark::State& state )
	{
		using vec = mclo::packed_int_vector<BitWidth, UnderlyingType>;
		using value_type = typename vec::value_type;

		const auto count = static_cast<typename vec::size_type>( state.range( 0 ) );
		vec v( count, vec::max_value / 2 );
		for ( auto _ : state )
		{
			value_type sum{};
			for ( typename vec::size_type i = 0; i < count; ++i )
			{
				sum += v.get( i );
			}
			benchmark::DoNotOptimize( sum );
		}
	}

	// Crossing + byte-offset load
	BENCHMARK( packed_int_vector_for_each<7, std::size_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get_loop<7, std::size_t> )->Apply( packed_int_vector_size_args );

	// Non-crossing: for_each can extract N values per physical load
	BENCHMARK( packed_int_vector_for_each<8, std::size_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get_loop<8, std::size_t> )->Apply( packed_int_vector_size_args );

	// Crossing, no byte-offset load: fallback delegates to get()
	BENCHMARK( packed_int_vector_for_each<27, std::uint32_t> )->Apply( packed_int_vector_size_args );
	BENCHMARK( packed_int_vector_get_loop<27, std::uint32_t> )->Apply( packed_int_vector_size_args );
}
