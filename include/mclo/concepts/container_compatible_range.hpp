#pragma once

#include <concepts>
#include <ranges>

namespace mclo
{
	template <typename Rng, typename T>
	concept container_compatible_range =
		std::ranges::input_range<Rng> && std::convertible_to<std::ranges::range_reference_t<Rng>, T>;
}
