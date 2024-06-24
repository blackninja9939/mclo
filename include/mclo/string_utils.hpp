#pragma once

#include "algorithm.hpp"
#include "fnva1.hpp"
#include "numeric.hpp"
#include "type_traits.hpp"

#include <array>
#include <charconv>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mclo
{
	template <typename T, typename... FromCharArgs>
	[[nodiscard]] std::optional<T> from_string( const std::string_view str, FromCharArgs&&... args ) noexcept
	{
		T value;
		const std::from_chars_result result =
			std::from_chars( str.data(), str.data() + str.size(), value, std::forward<FromCharArgs>( args )... );

		if ( result.ec == std::errc{} )
		{
			return value;
		}
		return {};
	}

	template <typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( char* const first,
											  char* const last,
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		const auto [ ptr, ec ] = std::to_chars( first, last, value, std::forward<ToCharArgs>( args )... );

		if ( ec == std::errc{} )
		{
			return std::string_view( first, ptr - first );
		}
		return {};
	}

	template <std::size_t BufferSize, typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( std::array<char, BufferSize>& buffer,
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		return to_string( buffer.data(), buffer.data() + BufferSize, value, std::forward<ToCharArgs>( args )... );
	}

	template <std::size_t BufferSize, typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( char ( &buffer )[ BufferSize ],
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		return to_string( std::begin( buffer ), std::end( buffer ), value, std::forward<ToCharArgs>( args )... );
	}

	inline constexpr std::string_view whitespace_characters = " \n\f\t\r\v";
	inline constexpr std::string_view numeric_characters = "0123456789";
	inline constexpr std::string_view uppercase_characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	inline constexpr std::string_view lowercase_characters = "abcdefgihjklmnopqrstuvwxyz";
	inline constexpr std::string_view alphabet_characters = "abcdefgihjklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	inline constexpr std::string_view alphanumeric_characters =
		"abcdefgihjklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	[[nodiscard]] constexpr std::string_view trim_front(
		const std::string_view string, const std::string_view to_trim = whitespace_characters ) noexcept
	{
		const auto start = string.find_first_not_of( to_trim );
		if ( start == std::string_view::npos )
		{
			return {};
		}
		return string.substr( start );
	}

	[[nodiscard]] constexpr std::string_view trim_back(
		const std::string_view string, const std::string_view to_trim = whitespace_characters ) noexcept
	{
		const auto end = string.find_last_not_of( to_trim );
		if ( end == std::string_view::npos )
		{
			return {};
		}
		return string.substr( 0, end + 1 );
	}

	[[nodiscard]] constexpr std::string_view trim( const std::string_view string,
												   const std::string_view to_trim = whitespace_characters ) noexcept
	{
		auto start = string.find_first_not_of( to_trim );
		if ( start == std::string_view::npos )
		{
			return {};
		}

		const auto end = string.find_last_not_of( to_trim );
		if ( end == std::string_view::npos )
		{
			return string.substr( start );
		}

		return string.substr( start, end + 1 - start );
	}

	[[nodiscard]] constexpr bool is_digit( const char c ) noexcept
	{
		return c >= '0' && c <= '9';
	}
	[[nodiscard]] constexpr bool is_uppercase_letter( const char c ) noexcept
	{
		return c >= 'A' && c <= 'Z';
	}
	[[nodiscard]] constexpr bool is_lowercase_letter( const char c ) noexcept
	{
		return c >= 'a' && c <= 'z';
	}
	[[nodiscard]] constexpr bool is_letter( const char c ) noexcept
	{
		return is_uppercase_letter( c ) || is_lowercase_letter( c );
	}
	[[nodiscard]] constexpr bool is_alphanumeric( const char c ) noexcept
	{
		return is_digit( c ) || is_letter( c );
	}

	[[nodiscard]] constexpr char to_upper( const char c ) noexcept
	{
		if ( is_lowercase_letter( c ) )
		{
			return c - ( 'a' - 'A' );
		}
		return c;
	}
	[[nodiscard]] constexpr char to_lower( const char c ) noexcept
	{
		if ( is_uppercase_letter( c ) )
		{
			return c + ( 'a' - 'A' );
		}
		return c;
	}

	template <typename It>
	constexpr void to_upper( It first, It last ) noexcept
	{
		while ( first != last )
		{
			char& c = *first++;
			c = to_upper( c );
		}
	}

	template <typename It>
	constexpr void to_lower( It first, It last ) noexcept
	{
		while ( first != last )
		{
			char& c = *first++;
			c = to_lower( c );
		}
	}

	template <typename Container>
	constexpr void to_upper( Container& string ) noexcept
	{
		to_upper( std::begin( string ), std::end( string ) );
	}

	template <typename Container>
	constexpr void to_lower( Container& string ) noexcept
	{
		to_lower( std::begin( string ), std::end( string ) );
	}

	void to_upper( std::wstring& string ) noexcept;
	void to_lower( std::wstring& string ) noexcept;

	namespace detail
	{
		constexpr int compare_ignore_case( const char* lhs, const char* rhs, std::size_t size ) noexcept
		{
			while ( size-- > 0 )
			{
				const char lhs_char = to_lower( *lhs++ );
				const char rhs_char = to_lower( *rhs++ );
				const int result = lhs_char - rhs_char;
				if ( result != 0 )
				{
					return result;
				}
			}
			return 0;
		}
	}

	constexpr int compare_ignore_case( const std::string_view lhs, const std::string_view rhs ) noexcept
	{
		const std::size_t lhs_size = lhs.size();
		const std::size_t rhs_size = rhs.size();

		const int result = detail::compare_ignore_case( lhs.data(), rhs.data(), std::min( lhs_size, rhs_size ) );
		if ( result != 0 )
		{
			return result;
		}

		if ( lhs_size < rhs_size )
		{
			return -1;
		}
		if ( lhs_size > rhs_size )
		{
			return 1;
		}
		return 0;
	}

	template <typename CharT>
	void replace_all( std::basic_string<CharT>& string,
					  const std::basic_string_view<type_identity_t<CharT>> find,
					  const std::basic_string_view<type_identity_t<CharT>> replace ) noexcept
	{
		const std::size_t find_size = find.size();
		const std::size_t replace_size = replace.size();
		std::size_t index = 0;
		for ( ;; )
		{
			index = string.find( find, index );
			if ( index == std::basic_string<CharT>::npos )
			{
				break;
			}
			string.replace( index, find_size, replace );
			index += replace_size;
		}
	}

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
				const std::size_t size = mclo::accumulate(
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
	}

	template <typename String = std::string, typename It>
	[[nodiscard]] constexpr String join_string( It first, It last )
	{
		using value_type = typename std::iterator_traits<It>::value_type;

		if constexpr ( std::is_same_v<value_type, const char*> )
		{
			return detail::join_string_iterators<String>(
				first, last, []( const auto& string ) { return std::char_traits<char>::length( string ); } );
		}
		else
		{
			return detail::join_string_iterators<String>(
				first, last, []( const auto& string ) { return std::size( string ); } );
		}
	}

	template <typename String = std::string, typename Container>
	[[nodiscard]] constexpr String join_string( const Container& strings )
	{
		return join_string<String>( std::begin( strings ), std::end( strings ) );
	}

	template <typename String = std::string, typename... Strings>
	[[nodiscard]] constexpr String join_string( const Strings&... strings )
	{
		return detail::join_string_views<String>( std::string_view( strings )... );
	}

	[[nodiscard]] constexpr std::size_t string_hash( const std::string_view string ) noexcept
	{
		return mclo::fnv1a( string.data(), string.size() );
	}
	[[nodiscard]] constexpr std::size_t string_hash_ignore_case( const std::string_view string ) noexcept
	{
		return mclo::fnv1a( string.data(), string.size(), static_cast<char ( * )( char ) noexcept>( to_lower ) );
	}
}
