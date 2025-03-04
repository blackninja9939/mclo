#pragma once

#include "mclo/string/string_view_type.hpp"

#include <string_view>
#include <type_traits>
#include <iterator>
#include <concepts>
#include <numeric>

// todo(mc) rework join API, it should be joining with a delimiter
// concatenating things should be separate
// both should also handle things convertible to strings like numbers since we can convert in a small buffer to a string

namespace mclo
{
	namespace detail
	{
		template <typename String, typename... Strings>
		[[nodiscard]] constexpr String join_string_views( const Strings&... strings )
		{
			const std::size_t size = ( std::size( strings ) + ... );
			String result;
			result.reserve( size );
			( result.append( std::data( strings ), std::size( strings ) ), ... );
			return result;
		}

		template <typename String, typename It, typename StringLength>
		[[nodiscard]] constexpr String reserve_string_for( It first, It last, StringLength string_length )
		{
			String result;

			using iterator_category = typename std::iterator_traits<It>::iterator_category;
			if constexpr ( std::is_base_of_v<std::forward_iterator_tag, iterator_category> )
			{
				const std::size_t size = std::accumulate(
					first, last, std::size_t( 0 ), [ &string_length ]( const std::size_t current, const auto& string ) {
						return current + string_length( string );
					} );

				result.reserve( size );
			}

			return result;
		}

		template <typename String, typename It, typename StringLength>
		[[nodiscard]] constexpr String join_string_iterators( It first, It last, StringLength string_length )
		{
			using value_type = typename std::iterator_traits<It>::value_type;
			using string_value_type = typename String::value_type;

			String result = reserve_string_for<String>( first, last, string_length );
			while ( first != last )
			{
				if constexpr ( std::is_same_v<value_type, string_value_type> )
				{
					result.push_back( *first++ );
				}
				else
				{
					result.append( *first++ );
				}
			}
			return result;
		}

		template <typename String, typename It>
		constexpr bool is_iterators_over_literals =
			std::is_same_v<typename std::iterator_traits<It>::value_type, const string_char_t<String>*>;

		inline constexpr std::size_t default_literal_size_buffer_max = 16;
	}

	template <typename String = std::string,
			  std::size_t literal_size_buffer_max = detail::default_literal_size_buffer_max,
			  typename It>
		requires( detail::is_iterators_over_literals<String, It> )
	[[nodiscard]] constexpr String join_string( It first, It last )
	{
		using string_value_type = typename String::value_type;
		using traits_type = std::char_traits<string_value_type>;
		using iterator_category = typename std::iterator_traits<It>::iterator_category;

		if constexpr ( std::is_base_of_v<std::forward_iterator_tag, iterator_category> )
		{
			if ( std::distance( first, last ) > literal_size_buffer_max )
			{
				return detail::join_string_iterators<String>(
					first, last, []( const auto& string ) { return traits_type::length( string ); } );
			}

			std::array<std::basic_string_view<string_value_type>, literal_size_buffer_max> sizes{};
			auto sizes_it = sizes.begin();
			while ( first != last )
			{
				*sizes_it++ = *first++;
			}

			return detail::join_string_iterators<String>(
				sizes.begin(), sizes_it, []( const auto& string ) { return std::size( string ); } );
		}
		else
		{
			return detail::join_string_iterators<String>(
				first, last, []( const auto& string ) { return traits_type::length( string ); } );
		}
	}

	template <typename String = std::string, typename It>
		requires( !detail::is_iterators_over_literals<String, It> )
	[[nodiscard]] constexpr String join_string( It first, It last )
	{
		return detail::join_string_iterators<String>(
			first, last, []( const auto& string ) { return std::size( string ); } );
	}

	template <typename String = std::string,
			  std::size_t literal_size_buffer_max = detail::default_literal_size_buffer_max,
			  typename Container>
		requires( detail::is_iterators_over_literals<String, typename Container::const_iterator> )
	[[nodiscard]] constexpr String join_string( const Container& strings )
	{
		return join_string<String, literal_size_buffer_max>( std::begin( strings ), std::end( strings ) );
	}

	template <typename String = std::string, typename Container>
		requires( !detail::is_iterators_over_literals<String, typename Container::const_iterator> )
	[[nodiscard]] constexpr String join_string( const Container& strings )
	{
		return join_string<String>( std::begin( strings ), std::end( strings ) );
	}

	template <typename String = std::string, typename... Strings>
	[[nodiscard]] constexpr String join_string( const Strings&... strings )
	{
		return detail::join_string_views<String>( std::string_view( strings )... );
	}
}
