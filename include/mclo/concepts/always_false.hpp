#pragma once

namespace mclo
{
	/// @brief Value that is always false but dependent on a template for delayed instantiation
	/// @details Use in static_asserts instead of static_assert( false );
	/// @note This is not needed in C++23 onwards
	/// @tparam ...Ts
	template <typename... Ts>
	constexpr bool always_false = false;
}
