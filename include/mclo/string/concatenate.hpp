#pragma once

#include "mclo/concepts/arithmetic.hpp"
#include "mclo/string/charconv.hpp"
#include "mclo/string/string_view_type.hpp"

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		template <typename String, typename... Ts>
			requires( std::same_as<string_view_t<String>, Ts> && ... )
		constexpr void append_strings( String& out, const Ts&... strings )
		{
			const std::size_t size = ( strings.size() + ... );
			out.reserve( out.size() + size );
			( out.append( strings ), ... );
		}

		template <mclo::arithmetic T>
		struct number_converter
		{
			constexpr number_converter( const T value ) noexcept
			{
				m_size = static_cast<std::uint8_t>( mclo::to_string( buffer, value ).size() );
			}

			constexpr operator std::string_view() const noexcept
			{
				return std::string_view{ buffer, m_size };
			}

			char buffer[ 32 ];
			std::uint8_t m_size = 0;
		};

		// Do not format plain chars as numbers
		struct char_converter
		{
			constexpr operator std::string_view() const noexcept
			{
				return std::string_view{ &c, 1 };
			}

			char c;
		};

		template <std::convertible_to<std::string_view> T>
		constexpr T&& convert_string( T&& arg ) noexcept
		{
			return std::forward<T>( arg );
		}

		template <mclo::arithmetic T>
		constexpr number_converter<T> convert_string( const T arg ) noexcept
		{
			return { arg };
		}

		constexpr char_converter convert_string( const char arg ) noexcept
		{
			return { arg };
		}
	}

	template <typename String, typename T>
	constexpr void append_string( String& out, T&& arg )
	{
		// Optimize for a single object we append without additional reserve call
		out.append( std::string_view( detail::convert_string( std::forward<T>( arg ) ) ) );
	}

	template <typename String, typename... Ts>
	constexpr void append_string( String& out, Ts&&... args )
	{
		static_assert( sizeof...( Ts ) > 0 );
		detail::append_strings( out, std::string_view( detail::convert_string( std::forward<Ts>( args ) ) )... );
	}

	template <typename String = std::string, typename... Ts>
	[[nodiscard]] constexpr String concat_string( Ts&&... args )
	{
		String str;
		append_string( str, std::forward<Ts>( args )... );
		return str;
	}
}
