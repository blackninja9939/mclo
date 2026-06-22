#pragma once

#include "mclo/hash/constexpr_hash.hpp"
#include "mclo/platform/cpp_feature_compat.hpp"
#include "mclo/string/ascii_string_utils.hpp"
#include "mclo/string/string_view_type.hpp"

#include <string_view>

namespace mclo
{
	/// @brief Computes a compile-time-capable hash of a string-like value.
	/// @details Normalises @p string to a @c std::basic_string_view so that string literals exclude the null
	/// terminator and character pointers compute their own length.
	/// @tparam String A string, string view, character pointer, or character array type.
	/// @param string The string to hash.
	/// @param salt An optional seed mixed into the hash.
	/// @return The hash value.
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
			/// @brief User-defined literal computing the compile-time hash of a string literal, e.g. @c "name"_hs.
			[[nodiscard]] constexpr std::size_t operator"" _hs( const char* const str, const std::size_t len ) noexcept
			{
				return mclo::constexpr_hash( str, len );
			}
		}
	}

	/// @brief Computes a compile-time-capable hash of a string-like value, ignoring ASCII case.
	/// @tparam String A string, string view, character pointer, or character array type.
	/// @param string The string to hash.
	/// @param salt An optional seed mixed into the hash.
	/// @return The case-insensitive hash value, equal for strings differing only in ASCII case.
	template <typename String>
	[[nodiscard]] constexpr std::size_t string_hash_ignore_case( const String& string,
																 const std::size_t salt = 0 ) noexcept
	{
		const string_view_t<String> view{ string };
		return mclo::constexpr_hash(
			view.data(), view.size(), salt, static_cast<char ( * )( char ) noexcept>( to_lower ) );
	}

	/// @brief Transparent hash functor wrapping @ref string_hash, suitable for heterogeneous string lookup.
	struct string_hash_t
	{
		using is_transparent = void;

		template <typename String>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const String& string )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::string_hash( string );
		}
	};

	/// @brief Transparent hash functor wrapping @ref string_hash_ignore_case for case-insensitive string lookup.
	struct string_hash_ignore_case_t
	{
		using is_transparent = void;

		template <typename String>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr std::size_t operator()( const String& string )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::string_hash_ignore_case( string );
		}
	};
}
