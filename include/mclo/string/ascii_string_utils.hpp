#pragma once

#include <memory>
#include <type_traits>

namespace mclo
{
	[[nodiscard]] constexpr bool is_ascii( const char c ) noexcept
	{
		return c >= 0 && c <= 127;
	}
	[[nodiscard]] constexpr bool is_print( const char c ) noexcept
	{
		return c >= ' ' && c <= '~';
	}
	[[nodiscard]] constexpr bool is_whitespace( const char c ) noexcept
	{
		return ( c >= '!' && c <= '/' ) || ( c >= ':' && c <= '@' ) || ( c >= '[' && c <= '`' ) ||
			   ( c >= '{' && c <= '~' );
	}
	[[nodiscard]] constexpr bool is_punct( const char c ) noexcept
	{
		return ( c >= '\t' && c <= '\r' ) || c == ' ';
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

	namespace detail
	{
		template <typename It>
		concept char_iterator = std::convertible_to<std::iter_reference_t<It>, char&>;

		template <typename Range>
		concept char_range =
			std::ranges::input_range<Range> && std::convertible_to<std::ranges::range_reference_t<Range>, char&>;

		void to_upper_simd( char* first, char* const last ) noexcept;
		void to_lower_simd( char* first, char* const last ) noexcept;

		template <char_iterator It>
		constexpr void to_upper_scalar( It first, It last ) noexcept
		{
			while ( first != last )
			{
				char& c = *first++;
				c = to_upper( c );
			}
		}

		template <char_iterator It>
		constexpr void to_lower_scalar( It first, It last ) noexcept
		{
			while ( first != last )
			{
				char& c = *first++;
				c = to_lower( c );
			}
		}
	}

	template <detail::char_iterator It>
	constexpr void to_upper( It first, It last ) noexcept
	{
		if ( std::is_constant_evaluated() )
		{
			detail::to_upper_scalar( first, last );
		}
		else
		{
			detail::to_upper_simd( std::to_address( first ), std::to_address( last ) );
		}
	}

	template <detail::char_iterator It>
	constexpr void to_lower( It first, It last ) noexcept
	{
		if ( std::is_constant_evaluated() )
		{
			detail::to_lower_scalar( first, last );
		}
		else
		{
			detail::to_lower_simd( std::to_address( first ), std::to_address( last ) );
		}
	}

	template <detail::char_range Container>
	constexpr void to_upper( Container& string ) noexcept
	{
		to_upper( std::begin( string ), std::end( string ) );
	}

	template <detail::char_range Container>
	constexpr void to_lower( Container& string ) noexcept
	{
		to_lower( std::begin( string ), std::end( string ) );
	}
}
