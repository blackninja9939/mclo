#pragma once

#include "mclo/concepts/any_of_type.hpp"

#include <cinttypes>
#include <climits>

namespace mclo
{
	/// @brief Matches the standard signed integer types, excluding @c bool, character types, and extended integers.
	/// @tparam T The type to test, with any cv-qualifiers removed before matching.
	template <typename T>
	concept standard_signed_integral =
		any_of_type<std::remove_cv_t<T>, signed char, signed short, signed int, signed long, signed long long>;

	/// @brief Matches the standard unsigned integer types, excluding @c bool, character types, and extended integers.
	/// @tparam T The type to test, with any cv-qualifiers removed before matching.
	template <typename T>
	concept standard_unsigned_integral = any_of_type<std::remove_cv_t<T>,
													 unsigned char,
													 unsigned short,
													 unsigned int,
													 unsigned long,
													 unsigned long long>;

	/// @brief Matches the standard signed or unsigned integer types.
	/// @details Unlike @c std::integral this excludes @c bool, character types, and extended integer types,
	/// restricting to the core arithmetic integer types.
	/// @tparam T The type to test, with any cv-qualifiers removed before matching.
	template <typename T>
	concept standard_integral = standard_signed_integral<T> || standard_unsigned_integral<T>;

	/// @brief The smallest unsigned integer type with at least @p Bits bits of width.
	/// @tparam Bits The minimum number of bits the resulting type must be able to hold.
	// clang-format off
	template <std::size_t Bits>
	using uint_least_t =
		std::conditional_t<(Bits<= sizeof(std::uint_least8_t) * CHAR_BIT), std::uint_least8_t,
		std::conditional_t<(Bits<= sizeof(std::uint_least16_t) * CHAR_BIT), std::uint_least16_t,
		std::conditional_t<(Bits<= sizeof(std::uint_least32_t) * CHAR_BIT), std::uint_least32_t,
		std::uint_least64_t
	>>>;
	// clang-format on

	/// @brief The smallest signed integer type with at least @p Bits bits of width.
	/// @tparam Bits The minimum number of bits the resulting type must be able to hold.
	template <std::size_t Bits>
	using int_least_t = std::make_signed_t<uint_least_t<Bits>>;
}
