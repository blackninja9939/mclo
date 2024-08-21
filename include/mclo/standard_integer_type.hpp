#pragma once

#include "any_of_type.hpp"

#include <cinttypes>
#include <climits>

namespace mclo
{
	template <typename T>
	constexpr bool is_standard_integer =
		is_any_of_v<std::remove_cv_t<T>, signed char, signed short, signed int, signed long, signed long long>;

	template <typename T>
	constexpr bool is_standard_unsigned_integer = is_any_of_v<std::remove_cv_t<T>,
															  unsigned char,
															  unsigned short,
															  unsigned int,
															  unsigned long,
															  unsigned long long>;

	
	// clang-format off
	template <std::size_t Bits>
	using uint_least_t =
		std::conditional_t<(Bits<= sizeof(std::uint_least8_t) * CHAR_BIT), std::uint_least8_t,
		std::conditional_t<(Bits<= sizeof(std::uint_least16_t) * CHAR_BIT), std::uint_least16_t,
		std::conditional_t<(Bits<= sizeof(std::uint_least32_t) * CHAR_BIT), std::uint_least32_t,
		std::conditional_t<(Bits<= sizeof(std::uint_least64_t) * CHAR_BIT), std::uint_least64_t,
		void
	>>>>;
	// clang-format on

	template <std::size_t Bits>
	using int_least_t = std::make_signed_t<uint_least_t<Bits>>;
}
