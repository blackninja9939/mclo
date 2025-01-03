#pragma once

#include <cstddef>
#include <span>

namespace mclo
{
	template <typename T>
	concept hasher = requires( T& hasher ) {
		{ hasher.write( std::span<const std::byte>{} ) } noexcept;
		{ hasher.finish() } noexcept -> std::convertible_to<std::size_t>;
	};
}
