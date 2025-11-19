#pragma once

#include "mclo/enum/enum_map.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <numeric>
#include <ranges>

namespace mclo
{
	template <typename TEnum, typename TValue, TEnum SizeEnum, typename Traits>
	class enum_bi_map_generic
	{
	public:
		using index_type = mclo::uint_least_t<std::bit_width( static_cast<std::size_t>( SizeEnum ) )>;
		static constexpr index_type max_size = static_cast<index_type>( SizeEnum );
		using data_pair = std::pair<TEnum, TValue>;

		template <std::ranges::input_range Range>
		constexpr enum_bi_map_generic( Range&& init_data )
		{
			DEBUG_ASSERT( std::ranges::size( init_data ) == max_size, "Invalid size for enum_bi_map" );
			std::ranges::move( std::forward<Range>( init_data ), m_data_to_enum.begin() );
			Traits::setup( m_data_to_enum );

			for ( index_type index = 0; index < max_size; ++index )
			{
				const auto& [ e, str ] = m_data_to_enum[ index ];
				m_enum_to_data_index[ e ] = index;
			}
		}

		template <typename Key>
		[[nodiscard]] constexpr std::optional<TEnum> lookup_from_data( const Key& key ) const noexcept
		{
			const auto first = m_data_to_enum.begin();

			const auto it = Traits::search( m_data_to_enum, key );
			if ( it == m_data_to_enum.end() ) [[unlikely]]
			{
				return {};
			}

			return it->first;
		}

		[[nodiscard]] constexpr const TValue& lookup_from_enum( const TEnum e ) const noexcept
		{
			return m_data_to_enum[ m_enum_to_data_index[ e ] ].second;
		}

	private:
		std::array<data_pair, max_size> m_data_to_enum{};
		mclo::enum_map<TEnum, index_type, SizeEnum> m_enum_to_data_index;
	};

	namespace detail
	{
		inline constexpr auto pair_second = []( const auto& pair ) -> const auto& { return pair.second; };

		struct linear_search
		{
			template <typename Range>
			static constexpr void setup( Range&& )
			{
			}

			template <std::ranges::contiguous_range Range, typename Find>
			[[nodiscard]] static constexpr auto search( Range&& range, const Find& find )
			{
				return std::ranges::find( std::forward<Range>( range ), find, pair_second );
			}
		};

		struct binary_search
		{
			template <typename Range>
			static constexpr void setup( Range&& range )
			{
				std::ranges::sort( std::forward<Range>( range ), std::ranges::less{}, pair_second );
			}

			template <std::ranges::contiguous_range Range, typename Find>
			[[nodiscard]] static constexpr auto search( Range&& range, const Find& find )
			{
				return std::ranges::lower_bound( std::forward<Range>( range ), find, std::ranges::less{}, pair_second );
			}
		};
	}

	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_bi_map_linear = enum_bi_map_generic<TEnum, TValue, SizeEnum, detail::linear_search>;

	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_bi_map_binary = enum_bi_map_generic<TEnum, TValue, SizeEnum, detail::binary_search>;

	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_bi_map = std::conditional_t<( static_cast<std::size_t>( SizeEnum ) >= 20 ),
										   enum_bi_map_binary<TEnum, TValue, SizeEnum>,
										   enum_bi_map_linear<TEnum, TValue, SizeEnum>>;
}
