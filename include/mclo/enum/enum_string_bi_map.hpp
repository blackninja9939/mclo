#pragma once

#include "mclo/enum/enum_map.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/numeric/standard_integer_type.hpp"
#include "mclo/string/hash.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <numeric>

namespace mclo
{
	template <typename TEnum>
	using enum_string_pair = std::pair<TEnum, std::string_view>;

	template <typename TEnum, TEnum SizeEnum, typename Traits>
	class enum_string_bi_map_generic
	{
	public:
		using index_type = mclo::uint_least_t<std::bit_width( static_cast<std::size_t>( SizeEnum ) )>;
		static constexpr index_type max_size = static_cast<index_type>( SizeEnum );
		using data_pair = std::pair<TEnum, std::string_view>;

		template <std::ranges::input_range Range>
		constexpr enum_string_bi_map_generic( Range&& init_data )
		{
			DEBUG_ASSERT( std::ranges::size( init_data ) == max_size, "Invalid size for enum_string_bi_map" );
			std::ranges::move( std::forward<Range>( init_data ), m_string_to_enum.begin() );
			Traits::setup( m_string_to_enum.begin(), m_string_to_enum.end() );

			for ( index_type index = 0; index < max_size; ++index )
			{
				const auto& [ e, str ] = m_string_to_enum[ index ];
				m_enum_to_string_index[ e ] = index;
			}
		}

		[[nodiscard]] constexpr std::optional<TEnum> lookup_from_string( const std::string_view str ) const noexcept
		{
			const auto first = m_string_to_enum.begin();
			const auto last = m_string_to_enum.end();

			const auto it = Traits::search( first, last, str );
			if ( it == last ) [[unlikely]]
			{
				return {};
			}

			return it->first;
		}

		[[nodiscard]] constexpr std::string_view lookup_from_enum( const TEnum e ) const noexcept
		{
			return m_string_to_enum[ m_enum_to_string_index[ e ] ].second;
		}

	private:
		std::array<data_pair, max_size> m_string_to_enum{};
		mclo::enum_map<TEnum, index_type, SizeEnum> m_enum_to_string_index;
	};

	namespace detail
	{
		struct linear_search
		{
			template <typename It>
			static void setup( It, It )
			{
			}

			template <typename It>
			static It search( It first, It last, std::string_view str )
			{
				return std::find_if( first, last, [ &str ]( const auto& lhs ) { return lhs.second == str; } );
			}
		};

		struct binary_search
		{
			template <typename It>
			static void setup( It first, It last )
			{
				std::sort( first, last, []( const auto& lhs, const auto& rhs ) {
					return mclo::string_hash( lhs.second ) < mclo::string_hash( rhs.second );
				} );
			}

			template <typename It>
			[[nodiscard]] static It search( It first, It last, std::string_view str )
			{
				return std::lower_bound(
					first, last, str, []( const auto& lhs, const auto& rhs ) { return lhs.second < rhs; } );
			}
		};
	}

	template <typename TEnum, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_string_bi_map_linear = enum_string_bi_map_generic<TEnum, SizeEnum, detail::linear_search>;

	template <typename TEnum, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_string_bi_map_binary = enum_string_bi_map_generic<TEnum, SizeEnum, detail::binary_search>;

	template <typename TEnum, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_string_bi_map = std::conditional_t<( static_cast<std::size_t>( SizeEnum ) >= 20 ),
												  enum_string_bi_map_binary<TEnum, SizeEnum>,
												  enum_string_bi_map_linear<TEnum, SizeEnum>>;
}
