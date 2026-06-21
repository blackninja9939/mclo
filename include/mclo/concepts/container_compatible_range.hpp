#pragma once

#include <concepts>
#include <ranges>

namespace mclo
{
	/// @brief Requires that Rng is an input range whose reference type is convertible to T
	template <typename Rng, typename T>
	concept container_compatible_range =
		std::ranges::input_range<Rng> && std::convertible_to<std::ranges::range_reference_t<Rng>, T>;
}
