#pragma once

#include "mclo/numeric/fixed_point.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <istream>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		/// @brief Base of the decimal representation produced and consumed by the textual conversions
		inline constexpr int decimal_radix = 10;

		/// @brief Largest number of fractional digits any fixed_point formatter will emit, bounding stack buffers
		inline constexpr int fixed_point_max_print_digits = 40;

		/// @brief Maximum characters a fixed_point of this configuration can produce: a sign, the integer digits, a
		/// decimal point and the larger of its own fractional digits or the formatter digit cap
		template <std::integral Rep, int Fraction>
		inline constexpr std::size_t fixed_point_max_chars =
			1 + ( std::numeric_limits<std::make_unsigned_t<Rep>>::digits10 + 1 ) + 1 +
			static_cast<std::size_t>( Fraction > fixed_point_max_print_digits ? Fraction
																			  : fixed_point_max_print_digits );

		/// @brief Return the magnitude of a possibly negative value as its unsigned counterpart
		///
		/// @details Negating through the unsigned type is well defined even for the most negative value, which has no
		/// positive counterpart in the signed type.
		template <std::integral Rep>
		[[nodiscard]] constexpr std::make_unsigned_t<Rep> fixed_point_magnitude( const Rep value ) noexcept
		{
			using unsigned_rep = std::make_unsigned_t<Rep>;
			const unsigned_rep bits = static_cast<unsigned_rep>( value );
			return value < 0 ? static_cast<unsigned_rep>( unsigned_rep{ 0 } - bits ) : bits;
		}

		/// @brief Add one to the decimal digit string digits[0, count), carrying through any trailing nines
		///
		/// @return true if the carry propagated past the most significant digit, so the integer part must be
		/// incremented
		[[nodiscard]] constexpr bool fixed_point_carry_increment( char* const digits, int count ) noexcept
		{
			while ( count > 0 )
			{
				char& digit = digits[ --count ];
				if ( digit != '9' )
				{
					++digit;
					return false;
				}
				digit = '0';
			}
			return true;
		}

		/// @brief Fill digits[0, Fraction) with the exact zero padded decimal digits of the fractional part of value
		///
		/// @details The fractional part of any fixed_point terminates within Fraction decimal digits (a binary scale of
		/// 2^F equals 5^F / 10^F and a decimal scale is already a power of ten), so long division by the scaling factor
		/// reproduces those digits exactly for either base while only ever holding remainder * 10 in the wider type.
		template <std::integral Rep, int Fraction, fixed_point_base Base>
		void fixed_point_fill_fraction( std::array<char, Fraction>& digits,
										const fixed_point<Rep, Fraction, Base> value ) noexcept
		{
			using wide = fixed_point_wider_t<Rep>;
			constexpr wide scale = static_cast<wide>( fixed_point<Rep, Fraction, Base>::scale );

			wide remainder = fixed_point_magnitude( value.fractional_part() );
			for ( int i = 0; i < Fraction; ++i )
			{
				remainder *= decimal_radix;
				digits[ i ] = static_cast<char>( '0' + static_cast<int>( remainder / scale ) );
				remainder %= scale;
			}
		}

		/// @brief Render a fixed_point value into buffer as a decimal string, returning the character count
		///
		/// @details A negative precision emits the exact value with trailing zeros trimmed; a non-negative precision
		/// emits exactly that many fractional digits, rounding halves away from zero and padding with zeros when more
		/// digits are requested than the value holds. The integer part is written with std::to_chars and the fractional
		/// digits come from fixed_point_fill_fraction.
		template <std::integral Rep, int Fraction, fixed_point_base Base>
		[[nodiscard]] std::size_t fixed_point_to_chars( std::array<char, fixed_point_max_chars<Rep, Fraction>>& buffer,
														const fixed_point<Rep, Fraction, Base> value,
														const int precision ) noexcept
		{
			char* const last = buffer.data() + buffer.size();
			char* out = buffer.data();
			if constexpr ( std::is_signed_v<Rep> )
			{
				if ( value.underlying() < 0 )
				{
					*out++ = '-';
				}
			}

			auto whole = fixed_point_magnitude( value.integer_part() );

			if constexpr ( Fraction == 0 )
			{
				out = std::to_chars( out, last, whole ).ptr;
				return static_cast<std::size_t>( out - buffer.data() );
			}
			else
			{
				std::array<char, Fraction> digits;
				fixed_point_fill_fraction( digits, value );

				// Choose how many fractional digits to emit and round the kept digits half away from zero
				int emit = Fraction;
				if ( precision < 0 )
				{
					while ( emit > 0 && digits[ emit - 1 ] == '0' )
					{
						--emit;
					}
				}
				else if ( precision < Fraction )
				{
					emit = precision;
					if ( digits[ precision ] >= '5' && fixed_point_carry_increment( digits.data(), precision ) )
					{
						++whole;
					}
				}

				out = std::to_chars( out, last, whole ).ptr;

				const int pad = precision > Fraction ? precision - Fraction : 0;
				if ( emit + pad > 0 )
				{
					*out++ = '.';
					out = std::copy_n( digits.data(), emit, out );
					out = std::fill_n( out, pad, '0' );
				}
				return static_cast<std::size_t>( out - buffer.data() );
			}
		}

		/// @brief Parse a decimal string into a fixed_point value, returning false on malformed input
		///
		/// @details The integer part and the fractional digits are each parsed with std::from_chars; the fractional
		/// digits are then scaled into the representation, rounding the result half away from zero. Digits beyond the
		/// representation's resolution cannot change the stored value and are ignored.
		template <std::integral Rep, int Fraction, fixed_point_base Base>
		[[nodiscard]] bool fixed_point_from_chars( const std::string_view text,
												   fixed_point<Rep, Fraction, Base>& out ) noexcept
		{
			using wide = fixed_point_wider_t<Rep>;
			using unsigned_rep = std::make_unsigned_t<Rep>;
			constexpr wide scale = static_cast<wide>( fixed_point<Rep, Fraction, Base>::scale );
			constexpr int max_fraction_digits = std::numeric_limits<unsigned_rep>::digits10;

			const char* cursor = text.data();
			const char* const last = cursor + text.size();
			if ( cursor == last )
			{
				return false;
			}

			bool negative = false;
			if ( *cursor == '+' || *cursor == '-' )
			{
				negative = *cursor == '-';
				++cursor;
			}

			std::uint64_t integer_value = 0;
			const std::from_chars_result parsed = std::from_chars( cursor, last, integer_value );
			bool any_digit = parsed.ec == std::errc{};
			cursor = parsed.ptr;

			wide fraction = 0;
			if ( cursor != last && *cursor == '.' )
			{
				++cursor;
				const char* const fraction_begin = cursor;
				while ( cursor != last && *cursor >= '0' && *cursor <= '9' )
				{
					++cursor;
				}
				const int provided = static_cast<int>( cursor - fraction_begin );
				if ( provided > 0 )
				{
					any_digit = true;
					const int kept = provided < max_fraction_digits ? provided : max_fraction_digits;
					std::uint64_t fraction_value = 0;
					std::from_chars( fraction_begin, fraction_begin + kept, fraction_value );

					const wide divisor = fixed_point_int_pow<wide>( decimal_radix, kept );
					fraction = ( static_cast<wide>( fraction_value ) * scale + divisor / 2 ) / divisor;
				}
			}

			if ( !any_digit || cursor != last )
			{
				return false;
			}

			const wide combined = static_cast<wide>( integer_value ) * scale + fraction;
			const unsigned_rep magnitude = static_cast<unsigned_rep>( combined );
			const unsigned_rep stored =
				negative ? static_cast<unsigned_rep>( unsigned_rep{ 0 } - magnitude ) : magnitude;
			out = fixed_point<Rep, Fraction, Base>( from_underlying, static_cast<Rep>( stored ) );
			return true;
		}
	}

	/// @brief Write a fixed_point value to a stream as its exact decimal value, honouring field width and fill
	template <typename CharT, typename Traits, std::integral Rep, int Fraction, fixed_point_base Base>
	std::basic_ostream<CharT, Traits>& operator<<( std::basic_ostream<CharT, Traits>& os,
												   const fixed_point<Rep, Fraction, Base> value )
	{
		std::array<char, detail::fixed_point_max_chars<Rep, Fraction>> buffer;
		const std::size_t length = detail::fixed_point_to_chars( buffer, value, -1 );
		if constexpr ( std::is_same_v<CharT, char> )
		{
			return os << std::string_view( buffer.data(), length );
		}
		else
		{
			CharT wide_buffer[ detail::fixed_point_max_chars<Rep, Fraction> ];
			for ( std::size_t i = 0; i < length; ++i )
			{
				wide_buffer[ i ] = static_cast<CharT>( buffer[ i ] );
			}
			return os << std::basic_string_view<CharT>( wide_buffer, length );
		}
	}

	/// @brief Read a fixed_point value from a stream, parsing the decimal text exactly into the representation
	template <typename CharT, typename Traits, std::integral Rep, int Fraction, fixed_point_base Base>
	std::basic_istream<CharT, Traits>& operator>>( std::basic_istream<CharT, Traits>& is,
												   fixed_point<Rep, Fraction, Base>& value )
	{
		std::basic_string<CharT, Traits> token;
		if ( is >> token )
		{
			std::string narrow;
			narrow.reserve( token.size() );
			for ( const CharT character : token )
			{
				narrow.push_back( static_cast<char>( character ) );
			}

			fixed_point<Rep, Fraction, Base> parsed;
			if ( detail::fixed_point_from_chars<Rep, Fraction, Base>( narrow, parsed ) )
			{
				value = parsed;
			}
			else
			{
				is.setstate( std::ios_base::failbit );
			}
		}
		return is;
	}
}

/// @brief Format a fixed_point value as its exact decimal value
///
/// @details With no specification the exact value is emitted with trailing zeros trimmed. An optional precision emits
/// that many fractional digits, rounding halves away from zero.
template <std::integral Rep, int Fraction, mclo::fixed_point_base Base, typename CharT>
struct std::formatter<mclo::fixed_point<Rep, Fraction, Base>, CharT>
{
	int precision = -1;

	constexpr typename std::basic_format_parse_context<CharT>::iterator parse(
		std::basic_format_parse_context<CharT>& ctx )
	{
		auto it = ctx.begin();
		const auto end = ctx.end();
		if ( it != end && *it == CharT( '.' ) )
		{
			++it;
			int value = 0;
			bool any = false;
			while ( it != end && *it >= CharT( '0' ) && *it <= CharT( '9' ) )
			{
				value = value * 10 + static_cast<int>( *it - CharT( '0' ) );
				any = true;
				++it;
			}
			if ( !any )
			{
				throw std::format_error( "fixed_point format precision requires at least one digit" );
			}
			precision =
				value > mclo::detail::fixed_point_max_print_digits ? mclo::detail::fixed_point_max_print_digits : value;
		}
		if ( it != end && *it != CharT( '}' ) )
		{
			throw std::format_error( "invalid format specification for fixed_point" );
		}
		return it;
	}

	template <typename FormatContext>
	typename FormatContext::iterator format( const mclo::fixed_point<Rep, Fraction, Base> value,
											 FormatContext& ctx ) const
	{
		std::array<char, mclo::detail::fixed_point_max_chars<Rep, Fraction>> buffer;
		const std::size_t length = mclo::detail::fixed_point_to_chars( buffer, value, precision );
		auto out = ctx.out();
		for ( std::size_t i = 0; i < length; ++i )
		{
			*out++ = static_cast<CharT>( buffer[ i ] );
		}
		return out;
	}
};
