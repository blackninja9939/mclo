#pragma once

#include "mclo/hash/constexpr_hash.hpp"
#include "mclo/preprocessor/platform.hpp"
#include "mclo/string/ascii_string_utils.hpp"
#include "mclo/string/string_view_type.hpp"

#include <string_view>

namespace mclo
{
	template <typename String>
	[[nodiscard]] constexpr std::size_t string_hash( const String& string, const std::size_t salt = 0 ) noexcept
	{
		// Necessary so literals do not include null terminator and so char*'s calculate their length
		const string_view_t<String> view{ string };
		return mclo::constexpr_hash( view.data(), view.size(), salt );
	}

	inline namespace literals
	{
		inline namespace hashed_string_literals
		{
			[[nodiscard]] constexpr std::size_t operator"" _hs( const char* const str, const std::size_t len ) noexcept
			{
				return mclo::constexpr_hash( str, len );
			}
		}
	}

	template <typename String>
	[[nodiscard]] constexpr std::size_t string_hash_ignore_case( const String& string,
																 const std::size_t salt = 0 ) noexcept
	{
		const string_view_t<String> view{ string };
		return mclo::constexpr_hash(
			view.data(), view.size(), salt, static_cast<char ( * )( char ) noexcept>( to_lower ) );
	}

	struct string_hash_t
	{
		template <typename String>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const String& string )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::string_hash( string );
		}
	};

	struct string_hash_ignore_case_t
	{
		template <typename String>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const String& string )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::string_hash_ignore_case( string );
		}
	};
}
