#include <benchmark/benchmark.h>

#include "mclo/random/chacha.hpp"
#include "mclo/random/splitmix64.hpp"
#include "mclo/random/xoshiro256plusplus.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <random>

namespace
{
	// Minimal wrapper around the C library's srand/rand for comparison against the library's generators.
	struct c_rand_engine
	{
		using result_type = int;

		explicit c_rand_engine( const result_type seed ) noexcept
		{
			std::srand( static_cast<unsigned int>( seed ) );
		}

		result_type operator()() const noexcept
		{
			return std::rand();
		}
	};

	constexpr std::uint64_t constant_seed = 0x0123456789abcdef;

	template <typename Engine>
	struct seeded_engine_factory
	{
		[[nodiscard]] static Engine make()
		{
			return Engine( static_cast<typename Engine::result_type>( constant_seed ) );
		}
	};

	template <std::size_t Rounds>
	struct seeded_engine_factory<mclo::chacha<Rounds>>
	{
		[[nodiscard]] static mclo::chacha<Rounds> make()
		{
			std::array<std::uint8_t, 32> key{};
			std::array<std::uint8_t, 12> nonce{};
			for ( std::size_t i = 0; i < key.size(); ++i )
			{
				key[ i ] = static_cast<std::uint8_t>( constant_seed >> ( ( i % sizeof( constant_seed ) ) * 8 ) );
			}
			return mclo::chacha<Rounds>( key, nonce );
		}
	};

	// Number of generator invocations per timing-loop iteration. Batching amortises the loop and measurement
	// overhead, which would otherwise be a significant fraction of the per-value cost for the fastest engines.
	constexpr int batch_size = 32;

	template <typename Engine>
	void generate_numbers( benchmark::State& state )
	{
		Engine engine = seeded_engine_factory<Engine>::make();

		for ( auto _ : state )
		{
			for ( int i = 0; i < batch_size; ++i )
			{
				auto result = engine();
				benchmark::DoNotOptimize( result );
			}
		}

		state.SetItemsProcessed( state.iterations() * batch_size );
	}
	BENCHMARK( generate_numbers<mclo::splitmix64> );
	BENCHMARK( generate_numbers<mclo::xoshiro256plusplus> );
	BENCHMARK( generate_numbers<mclo::chacha8> );
	BENCHMARK( generate_numbers<mclo::chacha12> );
	BENCHMARK( generate_numbers<mclo::chacha20> );
	BENCHMARK( generate_numbers<std::mt19937> );
	BENCHMARK( generate_numbers<std::mt19937_64> );
	BENCHMARK( generate_numbers<c_rand_engine> );
}
