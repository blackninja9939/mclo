#pragma once

namespace mclo
{
	/// @brief Disambiguation tag for constructing a wrapper type directly from its raw underlying representation,
	/// bypassing any scaling or clamping the value constructors would normally apply
	struct from_underlying_t
	{
		explicit from_underlying_t() = default;
	};

	/// @brief Tag value to select construction from a raw underlying representation
	inline constexpr from_underlying_t from_underlying{};
}
