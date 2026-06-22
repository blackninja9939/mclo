#pragma once

#include "mclo/container/span.hpp"

#include <cstddef>

namespace mclo
{
	/// @brief Concept for a stateful, incremental hashing algorithm used throughout the @c mclo hashing framework.
	/// @details A hasher accumulates bytes via repeated @c write calls and produces the final hash with @c finish.
	/// Both operations must be @c noexcept. Types are fed into a hasher through @ref hash_append, decoupling the
	/// hashing algorithm from the types being hashed (the "Types Don't Know #" design).
	/// @tparam T The candidate hasher type.
	template <typename T>
	concept hasher = requires( T& hasher ) {
		{ hasher.write( mclo::span<const std::byte>{} ) } noexcept;
		{ hasher.finish() } noexcept -> std::convertible_to<std::size_t>;
	};
}
