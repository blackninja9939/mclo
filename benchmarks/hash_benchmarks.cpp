#include <benchmark/benchmark.h>

#include "mclo/hash/hash_append.hpp"
#include "mclo/hash/hash_append_range.hpp"

#include "mclo/hash/constexpr_hash.hpp"
#include "mclo/hash/fnv1a_hasher.hpp"
#include "mclo/hash/murmur_hash_3.hpp"
#include "mclo/hash/rapidhash.hpp"

#include "mclo/hash/std_types.hpp"

#include "mclo/random/default_random_generator.hpp"

namespace
{
	template <typename T>
	void hash_combine( std::size_t& seed, const T& v )
	{
		static_assert( sizeof( std::size_t ) == 8 || sizeof( std::size_t ) == 4 );

		if constexpr ( sizeof( std::size_t ) == 8 )
		{
			std::size_t x = seed + 0x9e3779b9 + std::hash<T>()( v );
			const std::size_t m = 0xe9846af9b1a615d;
			x ^= x >> 32;
			x *= m;
			x ^= x >> 32;
			x *= m;
			x ^= x >> 28;
			seed = x;
		}
		else
		{
			std::size_t x = seed + 0x9e3779b9 + std::hash<T>()( v );
			const std::size_t m1 = 0x21f0aaad;
			const std::size_t m2 = 0x735a2d97;
			x ^= x >> 16;
			x *= m1;
			x ^= x >> 15;
			x *= m2;
			x ^= x >> 15;
			seed = x;
		}
	}

	template <typename Hasher>
	struct hash_helper
	{
		Hasher hasher;

		template <typename T>
		void object( const T& value ) noexcept
		{
			mclo::hash_append( hasher, value );
		}

		template <typename T>
		void range( T&& value ) noexcept
		{
			mclo::hash_append_range( hasher, std::forward<T>( value ) );
		}

		[[nodiscard]] std::size_t finish() noexcept
		{
			return hasher.finish();
		}
	};

	struct std_hash_helper
	{
		std::size_t hash = 0;

		template <typename T>
		void object( const T& value ) noexcept
		{
			hash_combine( hash, std::hash<T>()( value ) );
		}

		template <typename T>
		void range( T&& value ) noexcept
		{
			for ( auto&& v : std::forward<T>( value ) )
			{
				hash_combine( hash, std::hash<std::remove_cvref_t<decltype( v )>>()( v ) );
			}
		}

		[[nodiscard]] std::size_t finish() const noexcept
		{
			return hash;
		}
	};

	template <typename Helper>
	void hash_benchmark( benchmark::State& state )
	{
		mclo::default_random_generator random;

		std::vector<int> vec( 50 );
		random.generate( vec );

		std::vector<std::string> strs( 50 );
		std::ranges::transform( vec, strs.begin(), []( int i ) { return std::to_string( i ); } );
		std::ranges::shuffle( vec, random.get_engine() );

		for ( auto _ : state )
		{
			Helper helper;
			helper.object( nullptr );
			helper.object( random.uniform( 0, 100 ) );
			helper.range( vec );
			helper.range( strs );
			std::size_t hash = helper.finish();
			benchmark::DoNotOptimize( hash );
		}
	}
	BENCHMARK( hash_benchmark<hash_helper<mclo::fnv1a_hasher>> );
	BENCHMARK( hash_benchmark<hash_helper<mclo::murmur_hash_3>> );
	BENCHMARK( hash_benchmark<hash_helper<mclo::rapidhash>> );
	BENCHMARK( hash_benchmark<std_hash_helper> );
}
