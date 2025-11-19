#include <benchmark/benchmark.h>

#include "mclo/debug/assert.hpp"
#include "mclo/enum/enum_bi_map.hpp"
#include "mclo/hash/constexpr_hash.hpp"

#include <charconv>
#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

namespace
{
	enum class test_enum
	{
	};

	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	class enum_bi_map_unordered_map
	{
	public:
		static constexpr std::size_t max_size = static_cast<std::size_t>( SizeEnum );
		using data_pair = std::pair<TEnum, TValue>;

		template <std::ranges::input_range Range>
		constexpr enum_bi_map_unordered_map( Range&& init_data )
		{
			DEBUG_ASSERT( std::ranges::size( init_data ) == max_size, "Invalid size for enum_bi_map_unordered_map" );
			m_data_to_enum.reserve( max_size );
			m_enum_to_data.reserve( max_size );
			for ( const auto& [ e, str ] : init_data )
			{
				m_data_to_enum.emplace( str, e );
				m_enum_to_data.emplace( e, str );
			}
		}

		constexpr std::optional<TEnum> lookup_from_data( const TValue& data ) const noexcept
		{
			const auto it = m_data_to_enum.find( data );
			if ( it == m_data_to_enum.end() )
			{
				return {};
			}
			return it->second;
		}

		constexpr const TValue& lookup_from_enum( const TEnum e ) const noexcept
		{
			return m_enum_to_data.find( e )->second;
		}

	private:
		std::unordered_map<TValue, TEnum> m_data_to_enum;
		std::unordered_map<TEnum, TValue> m_enum_to_data; // for a real once we'd use the enum map ehre too
	};

	template <std::size_t size>
	auto make_map_init()
	{
		using str_array = std::array<char, 20>;
		std::vector<str_array> strs( size );
		std::vector<std::pair<test_enum, std::string_view>> arr( size );

		int i = 0;
		for ( auto& [ e, str ] : arr )
		{
			e = ( test_enum )i;

			str_array& hashStr = strs[ i ];
			const std::size_t hash = mclo::constexpr_hash( &i, 1 );
			const std::to_chars_result result = std::to_chars( hashStr.data(), hashStr.data() + hashStr.size(), hash );
			DEBUG_ASSERT( result.ec == std::errc{}, "Parsing should succeed" );

			str = { hashStr.data(), result.ptr };
			++i;
		}

		return std::make_pair( std::move( strs ), std::move( arr ) );
	}

	template <template <typename E, typename V, E> typename BiMap, std::size_t size>
	void BM_EnumStringBiMap( benchmark::State& state )
	{
		static const auto setup = make_map_init<size>();
		const auto& arr = setup.second;
		const BiMap<test_enum, std::string_view, test_enum( size )> map( arr );

		std::mt19937 gen( 42 );
		std::uniform_int_distribution<std::size_t> dist( 0, arr.size() - 1 );

		for ( auto _ : state )
		{
			auto result = map.lookup_from_data( arr[ dist( gen ) ].second );
			benchmark::DoNotOptimize( result );
		}
	}

#define BENCHMARK_BI_MAP( TYPE )                                                                                       \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 5> );                                                                          \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 10> );                                                                         \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 20> );                                                                         \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 50> );                                                                         \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 100> );                                                                        \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 500> );                                                                        \
	BENCHMARK( BM_EnumStringBiMap<TYPE, 1000> )

	BENCHMARK_BI_MAP( enum_bi_map_unordered_map );
	BENCHMARK_BI_MAP( mclo::enum_bi_map_linear );
	BENCHMARK_BI_MAP( mclo::enum_bi_map_binary );

#undef BENCHMARK_BI_MAP
}
