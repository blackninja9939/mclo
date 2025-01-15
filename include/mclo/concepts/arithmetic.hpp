#pragma once

#include <type_traits>

namespace mclo
{
	/// @brief Requires that T is an arithmetic (integral or floating point) type
	template <typename T>
	concept arithmetic = std::is_arithmetic_v<T>;
}
