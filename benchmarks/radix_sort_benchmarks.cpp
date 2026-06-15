#include <benchmark/benchmark.h>

#include "mclo/algorithm/radix_sort.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <vector>

namespace
{
	// --- Algorithm wrappers ---

	struct std_sort_algo
	{
		template <typename T, typename KeyExtractor>
		static mclo::radix_sort_result sort( std::vector<T>& source,
											 std::vector<T>& /*output*/,
											 KeyExtractor key_extractor )
		{
			std::sort( source.begin(), source.end(), [ & ]( const T& a, const T& b ) {
				return std::invoke( key_extractor, a ) < std::invoke( key_extractor, b );
			} );
			return mclo::radix_sort_result::in_source;
		}
	};

	struct radix_sort_algo
	{
		template <typename T, typename KeyExtractor>
		static mclo::radix_sort_result sort( std::vector<T>& source,
											 std::vector<T>& output,
											 KeyExtractor key_extractor )
		{
			return mclo::radix_sort( source.begin(), source.end(), output.begin(), key_extractor );
		}
	};

	// --- Element types ---

	template <typename KeyT>
	using keyed_element = std::pair<KeyT, std::uint32_t>;

	template <typename KeyT>
	using large_keyed_element = std::pair<KeyT, std::string>;

	struct extract_key
	{
		template <typename T>
		[[nodiscard]] auto operator()( const T& p ) const noexcept
		{
			return p.first;
		}
	};

	// --- Data generation ---

	template <typename T>
	void fill_data( T& out, std::mt19937& rng )
	{
		using dist_type = std::conditional_t<( sizeof( T ) < sizeof( int ) ), int, T>;
		out = static_cast<T>( std::uniform_int_distribution<dist_type>{}( rng ) );
	}

	void fill_data( bool& out, std::mt19937& rng )
	{
		out = std::uniform_int_distribution<int>{ 0, 1 }( rng ) == 1;
	}

	void fill_data( float& out, std::mt19937& rng )
	{
		out = std::uniform_real_distribution<float>{ -1e6f, 1e6f }( rng );
	}

	void fill_data( double& out, std::mt19937& rng )
	{
		out = std::uniform_real_distribution<double>{ -1e15, 1e15 }( rng );
	}

	void fill_data( std::string& out, std::mt19937& rng )
	{
		const auto len = std::uniform_int_distribution<int>{ 4, 32 }( rng );
		out.resize( static_cast<std::size_t>( len ) );
		for ( char& c : out )
		{
			c = static_cast<char>( std::uniform_int_distribution<int>{ 'a', 'z' }( rng ) );
		}
	}

	template <typename T, typename U>
	void fill_data( std::pair<T, U>& out, std::mt19937& rng )
	{
		fill_data( out.first, rng );
		fill_data( out.second, rng );
	}

	template <typename T>
	std::vector<T> generate_random_data( const std::size_t n )
	{
		std::mt19937 rng( 42 );
		using dist_type = std::conditional_t<( sizeof( T ) < sizeof( int ) ), int, T>;
		std::vector<T> data( n );
		for ( auto&& v : data )
		{
			fill_data( v, rng );
		}
		return data;
	}

	// --- Benchmark function ---

	template <typename Algorithm, typename T, typename KeyExtractor>
	void sort_benchmark( benchmark::State& state )
	{
		const auto n = static_cast<std::size_t>( state.range( 0 ) );
		const auto original = generate_random_data<T>( n );
		std::vector<T> source( n );
		std::vector<T> output( n );

		for ( auto _ : state )
		{
			source = original;
			const auto result = Algorithm::sort( source, output, KeyExtractor{} );
			if ( result == mclo::radix_sort_result::in_source )
			{
				output = std::move( source );
			}
			benchmark::DoNotOptimize( output );
			benchmark::ClobberMemory();
		}

		const auto items_processed = state.iterations() * n;
		state.SetItemsProcessed( items_processed );
		state.SetBytesProcessed( static_cast<std::int64_t>( items_processed * sizeof( T ) ) );
	}

	void sizes( benchmark::internal::Benchmark* b )
	{
		b->RangeMultiplier( 2 )->Range( 16, 1 << 16 );
	}

#define BENCHMARK_GROUP( type, key_extractor )                                                                         \
	BENCHMARK( sort_benchmark<std_sort_algo, type, key_extractor> )->Apply( sizes )->Complexity( benchmark::oNLogN );  \
	BENCHMARK( sort_benchmark<radix_sort_algo, type, key_extractor> )->Apply( sizes )->Complexity( benchmark::oN )

#define BENCHMARK_GROUP_TYPED( type )                                                                                  \
	BENCHMARK_GROUP( type, std::identity );                                                                            \
	BENCHMARK_GROUP( keyed_element<type>, extract_key );                                                               \
	BENCHMARK_GROUP( large_keyed_element<type>, extract_key );

	BENCHMARK_GROUP( keyed_element<bool>, extract_key );
	BENCHMARK_GROUP( large_keyed_element<bool>, extract_key );
	BENCHMARK_GROUP_TYPED( std::uint8_t );
	BENCHMARK_GROUP_TYPED( std::int8_t );
	BENCHMARK_GROUP_TYPED( std::uint16_t );
	BENCHMARK_GROUP_TYPED( std::int16_t );
	BENCHMARK_GROUP_TYPED( std::uint32_t );
	BENCHMARK_GROUP_TYPED( std::int32_t );
	BENCHMARK_GROUP_TYPED( std::uint64_t );
	BENCHMARK_GROUP_TYPED( std::int64_t );
	BENCHMARK_GROUP_TYPED( float );
	BENCHMARK_GROUP_TYPED( double );
}
