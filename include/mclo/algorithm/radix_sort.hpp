#pragma once

#include <array>
#include <bit>
#include <climits>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>

namespace mclo
{
	/// @brief Indicates which buffer contains the sorted output after a radix sort.
	enum class radix_sort_result : bool
	{
		/// @brief The sorted data is in the source range.
		in_source,

		/// @brief The sorted data is in the output range.
		in_output,
	};

	namespace detail
	{
		template <std::unsigned_integral T>
		[[nodiscard]] constexpr T to_radix_key( const T value ) noexcept
		{
			return value;
		}

		template <std::signed_integral T>
		[[nodiscard]] constexpr auto to_radix_key( const T value ) noexcept
		{
			using unsigned_t = std::make_unsigned_t<T>;
			constexpr unsigned_t sign_bit = unsigned_t{ 1 } << ( sizeof( T ) * CHAR_BIT - 1 );
			return static_cast<unsigned_t>( value ) ^ sign_bit;
		}

		[[nodiscard]] constexpr std::uint32_t to_radix_key( const float value ) noexcept
		{
			const std::uint32_t bits = std::bit_cast<std::uint32_t>( value );
			const std::uint32_t mask = -static_cast<std::int32_t>( bits >> 31 ) | 0x80000000u;
			return bits ^ mask;
		}

		[[nodiscard]] constexpr std::uint64_t to_radix_key( const double value ) noexcept
		{
			const std::uint64_t bits = std::bit_cast<std::uint64_t>( value );
			const std::uint64_t mask = -static_cast<std::int64_t>( bits >> 63 ) | ( std::uint64_t{ 1 } << 63 );
			return bits ^ mask;
		}

		template <typename KeyExtractor, typename It>
		using extracted_key_t = std::decay_t<std::indirect_result_t<KeyExtractor, It>>;

		template <std::unsigned_integral CountT,
				  std::random_access_iterator It,
				  std::sentinel_for<It> Sentinel,
				  std::output_iterator<std::iter_value_t<It>> OutIt,
				  std::indirectly_unary_invocable<It> KeyExtractor = std::identity>
		void counting_sort_bool_impl( It first, Sentinel last, OutIt out, KeyExtractor key_extractor = {} )
		{
			CountT num_false{ 0 };
			for ( It it = first; it != last; ++it )
			{
				if ( !std::invoke( key_extractor, *it ) )
				{
					++num_false;
				}
			}

			OutIt true_it = out + num_false;
			for ( ; first != last; ++first )
			{
				if ( std::invoke( key_extractor, *first ) )
				{
					*true_it++ = std::move( *first );
				}
				else
				{
					*out++ = std::move( *first );
				}
			}
		}

		template <std::unsigned_integral CountT,
				  std::random_access_iterator It,
				  std::sentinel_for<It> Sentinel,
				  std::output_iterator<std::iter_value_t<It>> OutIt,
				  std::indirectly_unary_invocable<It> KeyExtractor = std::identity>
		void counting_sort_impl( It first, Sentinel last, OutIt out, KeyExtractor key_extractor = {} )
		{
			std::array<CountT, 256> count{};
			for ( It it = first; it != last; ++it )
			{
				const auto key = to_radix_key( std::invoke( key_extractor, *it ) );
				++count[ key ];
			}

			CountT total = 0;
			for ( CountT& c : count )
			{
				const CountT old_count = c;
				c = total;
				total += old_count;
			}

			for ( ; first != last; ++first )
			{
				const auto key = to_radix_key( std::invoke( key_extractor, *first ) );
				out[ count[ key ]++ ] = std::move( *first );
			}
		}

		template <std::unsigned_integral CountT,
				  std::random_access_iterator It,
				  std::sentinel_for<It> Sentinel,
				  std::random_access_iterator OutIt,
				  std::indirectly_unary_invocable<It> KeyExtractor>
		[[nodiscard]] radix_sort_result radix_sort_impl( It first,
														 Sentinel last,
														 OutIt out,
														 KeyExtractor key_extractor )
		{
			using key_type = extracted_key_t<KeyExtractor, It>;
			constexpr std::size_t key_bytes = sizeof( decltype( to_radix_key( std::declval<key_type>() ) ) );

			const auto size = last - first;

			// Build all byte-level histograms in a single pass
			std::array<std::array<CountT, 256>, key_bytes> counts{};
			for ( It it = first; it != last; ++it )
			{
				auto key = to_radix_key( std::invoke( key_extractor, *it ) );
				for ( std::size_t byte_idx = 0; byte_idx < key_bytes; ++byte_idx )
				{
					++counts[ byte_idx ][ key & 0xFF ];
					key >>= 8;
				}
			}

			// Convert each histogram to prefix sums
			for ( auto& byte_counts : counts )
			{
				CountT total = 0;
				for ( CountT& c : byte_counts )
				{
					const CountT old_count = c;
					c = total;
					total += old_count;
				}
			}

			// Scatter passes, LSB first, ping-ponging between source and out
			// Even passes: [first, last) -> [out, out + size)
			// Odd passes:  [out, out + size) -> [first, last)
			for ( std::size_t byte_idx = 0; byte_idx < key_bytes; ++byte_idx )
			{
				auto& byte_counts = counts[ byte_idx ];
				const auto shift = static_cast<unsigned>( byte_idx * 8 );

				if ( ( byte_idx & 1 ) == 0 )
				{
					for ( auto it = first; it != last; ++it )
					{
						const auto key = to_radix_key( std::invoke( key_extractor, *it ) );
						const auto bucket = ( key >> shift ) & 0xFF;
						out[ byte_counts[ bucket ]++ ] = std::move( *it );
					}
				}
				else
				{
					for ( auto it = out; it != out + size; ++it )
					{
						const auto key = to_radix_key( std::invoke( key_extractor, *it ) );
						const auto bucket = ( key >> shift ) & 0xFF;
						first[ byte_counts[ bucket ]++ ] = std::move( *it );
					}
				}
			}

			if constexpr ( key_bytes % 2 == 0 )
			{
				return radix_sort_result::in_source;
			}
			else
			{
				return radix_sort_result::in_output;
			}
		}
	}

	/// @brief Sorts elements by a 1-byte or boolean key using a counting sort.
	/// @details Always writes the sorted result to the output range. For boolean keys a
	/// specialized two-bucket partition is used. For 1-byte integer keys a 256-bucket
	/// counting sort is used.
	/// @note Profile before committing to this over a comparison sort. Performance relative
	/// to std::sort improves with smaller element types and larger ranges.
	/// @tparam It Random access iterator to the source range.
	/// @tparam Sentinel Sentinel type for @p It.
	/// @tparam OutIt Output iterator to the destination range.
	/// @tparam KeyExtractor Unary invocable that extracts a sortable key from each element.
	/// @param first Iterator to the first element of the source range.
	/// @param last Sentinel denoting the end of the source range.
	/// @param out Iterator to the first element of the destination range.
	/// @param key_extractor Functor that extracts the sort key from each element.
	/// @pre The destination range must be at least as large as the source range.
	/// @pre The source and destination ranges must not overlap.
	template <std::random_access_iterator It,
			  std::sentinel_for<It> Sentinel,
			  std::output_iterator<std::iter_value_t<It>> OutIt,
			  std::indirectly_unary_invocable<It> KeyExtractor = std::identity>
	void counting_sort( It first, Sentinel last, OutIt out, KeyExtractor key_extractor = {} )
	{
		using key_type = detail::extracted_key_t<KeyExtractor, It>;

		const auto size = last - first;

		if constexpr ( std::same_as<key_type, bool> )
		{
			if ( size <= std::numeric_limits<std::uint8_t>::max() )
			{
				detail::counting_sort_bool_impl<std::uint8_t>( first, last, out, key_extractor );
			}
			else if ( size <= std::numeric_limits<std::uint16_t>::max() )
			{
				detail::counting_sort_bool_impl<std::uint16_t>( first, last, out, key_extractor );
			}
			else if ( size <= std::numeric_limits<std::uint32_t>::max() )
			{
				detail::counting_sort_bool_impl<std::uint32_t>( first, last, out, key_extractor );
			}
			else
			{
				detail::counting_sort_bool_impl<std::uint64_t>( first, last, out, key_extractor );
			}
		}
		else
		{
			static_assert( sizeof( key_type ) == 1, "counting_sort only supports 1-byte key types" );

			if ( size <= std::numeric_limits<std::uint8_t>::max() )
			{
				detail::counting_sort_impl<std::uint8_t>( first, last, out, key_extractor );
			}
			else if ( size <= std::numeric_limits<std::uint16_t>::max() )
			{
				detail::counting_sort_impl<std::uint16_t>( first, last, out, key_extractor );
			}
			else if ( size <= std::numeric_limits<std::uint32_t>::max() )
			{
				detail::counting_sort_impl<std::uint32_t>( first, last, out, key_extractor );
			}
			else
			{
				detail::counting_sort_impl<std::uint64_t>( first, last, out, key_extractor );
			}
		}
	}

	/// @brief Sorts elements by key using an LSB radix sort.
	/// @details Performs a least-significant-byte-first radix sort, scattering elements between
	/// the source and output buffers on alternating passes. For 1-byte keys this delegates to
	/// @ref counting_sort. The returned @ref radix_sort_result indicates which buffer holds the
	/// sorted data after the final pass.
	/// @note Profile before committing to this over a comparison sort. Performance relative
	/// to std::sort improves with smaller element types, smaller key types, and larger ranges.
	/// The number of scatter passes scales linearly with the key size in bytes.
	/// @tparam It Random access iterator to the source range.
	/// @tparam Sentinel Sentinel type for @p It.
	/// @tparam OutIt Random access iterator to the destination range.
	/// @tparam KeyExtractor Unary invocable that extracts a sortable key from each element.
	/// @param first Iterator to the first element of the source range.
	/// @param last Sentinel denoting the end of the source range.
	/// @param out Iterator to the first element of the destination range.
	/// @param key_extractor Functor that extracts the sort key from each element.
	/// @return @ref radix_sort_result::in_output if the sorted data is in the output range,
	/// @ref radix_sort_result::in_source if it remains in the source range.
	/// @pre The destination range must be at least as large as the source range.
	/// @pre The source and destination ranges must not overlap.
	/// @warning The source range is modified regardless of the returned result.
	template <std::random_access_iterator It,
			  std::sentinel_for<It> Sentinel,
			  std::random_access_iterator OutIt,
			  std::indirectly_unary_invocable<It> KeyExtractor = std::identity>
	[[nodiscard]] radix_sort_result radix_sort( It first, Sentinel last, OutIt out, KeyExtractor key_extractor = {} )
	{
		using key_type = detail::extracted_key_t<KeyExtractor, It>;

		if constexpr ( sizeof( key_type ) == 1 )
		{
			counting_sort( first, last, out, key_extractor );
			return radix_sort_result::in_output;
		}
		else
		{
			const auto size = last - first;

			if ( size <= std::numeric_limits<std::uint8_t>::max() )
			{
				return detail::radix_sort_impl<std::uint8_t>( first, last, out, key_extractor );
			}
			else if ( size <= std::numeric_limits<std::uint16_t>::max() )
			{
				return detail::radix_sort_impl<std::uint16_t>( first, last, out, key_extractor );
			}
			else if ( size <= std::numeric_limits<std::uint32_t>::max() )
			{
				return detail::radix_sort_impl<std::uint32_t>( first, last, out, key_extractor );
			}
			else
			{
				return detail::radix_sort_impl<std::uint64_t>( first, last, out, key_extractor );
			}
		}
	}
}
