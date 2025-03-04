#pragma once

#include "mclo/container/span.hpp"

#include <cstddef>

namespace mclo
{
	template <typename T>
	concept hasher = requires( T& hasher ) {
		{ hasher.write( mclo::span<const std::byte>{} ) } noexcept;
		{ hasher.finish() } noexcept -> std::convertible_to<std::size_t>;
	};
}
