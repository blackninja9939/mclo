#pragma once

#include "mclo/enum/enum_map.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/numeric/standard_integer_type.hpp"
#include "mclo/platform/cpp_feature_compat.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <numeric>
#include <optional>
#include <ranges>

namespace mclo
{
	/// @brief A fixed-size bidirectional map between every enumerator of @p TEnum and an associated value.
	/// @details Supports lookup in both directions: an enumerator to its value (O(1) array index) and a value back to
	/// its enumerator (via the search strategy supplied by @p Traits). The full mapping is provided once at
	/// construction and is immutable thereafter. Prefer the @ref enum_bi_map, @ref enum_bi_map_linear, or
	/// @ref enum_bi_map_binary aliases rather than naming this template directly.
	/// @tparam TEnum The enumeration type.
	/// @tparam TValue The value associated with each enumerator.
	/// @tparam SizeEnum The number of enumerators.
	/// @tparam Traits The search policy providing @c setup and @c search for the value-to-enumerator direction.
	template <typename TEnum, typename TValue, TEnum SizeEnum, typename Traits>
	class enum_bi_map_generic
	{
	public:
		using index_type = mclo::uint_least_t<std::bit_width( static_cast<std::size_t>( SizeEnum ) )>;
		static constexpr index_type max_size = static_cast<index_type>( SizeEnum );
		using data_pair = std::pair<TEnum, TValue>;

		/// @brief Constructs the map from a range of exactly @p SizeEnum enumerator/value pairs.
		/// @param init_data The range of @ref data_pair entries defining the mapping.
		/// @pre @p init_data must contain exactly @ref max_size pairs, one per enumerator.
		template <std::ranges::input_range Range>
		constexpr enum_bi_map_generic( Range&& init_data )
		{
			MCLO_DEBUG_ASSERT( std::ranges::size( init_data ) == max_size, "Invalid size for enum_bi_map" );
			std::ranges::move( std::forward<Range>( init_data ), m_data_to_enum.begin() );
			Traits::setup( m_data_to_enum );

			for ( index_type index = 0; index < max_size; ++index )
			{
				const auto& [ e, str ] = m_data_to_enum[ index ];
				m_enum_to_data_index[ e ] = index;
			}
		}

		/// @brief Looks up the enumerator associated with @p key.
		/// @param key The value to search for.
		/// @return The matching enumerator, or an empty optional if @p key is not present.
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

		/// @brief Returns the value associated with the enumerator @p e.
		/// @param e The enumerator to look up.
		/// @return A reference to the associated value.
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
		struct pair_second
		{
			template <typename T, typename U>
			[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr const U& operator()( const std::pair<T, U>& pair )
				MCLO_CONST_CALL_OPERATOR
			{
				return pair.second;
			}
		};

		struct linear_search
		{
			template <typename Range>
			static constexpr void setup( Range&& )
			{
			}

			template <std::ranges::contiguous_range Range, typename Find>
			[[nodiscard]] static constexpr auto search( Range&& range, const Find& find )
			{
				return std::ranges::find( std::forward<Range>( range ), find, pair_second{} );
			}
		};

		struct binary_search
		{
			template <typename Range>
			static constexpr void setup( Range&& range )
			{
				std::ranges::sort( std::forward<Range>( range ), std::ranges::less{}, pair_second{} );
			}

			template <std::ranges::contiguous_range Range, typename Find>
			[[nodiscard]] static constexpr auto search( Range&& range, const Find& find )
			{
				return std::ranges::lower_bound(
					std::forward<Range>( range ), find, std::ranges::less{}, pair_second{} );
			}
		};
	}

	/// @brief An @ref enum_bi_map_generic using linear search for value-to-enumerator lookup.
	/// @details Best for small mappings where a scan is faster than the overhead of an ordered search.
	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_bi_map_linear = enum_bi_map_generic<TEnum, TValue, SizeEnum, detail::linear_search>;

	/// @brief An @ref enum_bi_map_generic using binary search for value-to-enumerator lookup.
	/// @details Sorts the entries by value at construction, making lookups O(log n); best for larger mappings.
	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_bi_map_binary = enum_bi_map_generic<TEnum, TValue, SizeEnum, detail::binary_search>;

	/// @brief A bidirectional enumerator/value map that picks a search strategy based on its size.
	/// @details Resolves to @ref enum_bi_map_binary for 20 or more enumerators and @ref enum_bi_map_linear otherwise,
	/// giving a sensible default for the common case.
	template <typename TEnum, typename TValue, TEnum SizeEnum = mclo::enum_size<TEnum>>
	using enum_bi_map = std::conditional_t<( static_cast<std::size_t>( SizeEnum ) >= 20 ),
										   enum_bi_map_binary<TEnum, TValue, SizeEnum>,
										   enum_bi_map_linear<TEnum, TValue, SizeEnum>>;
}
