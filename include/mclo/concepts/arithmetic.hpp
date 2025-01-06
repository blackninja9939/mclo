#pragma once

#include <type_traits>

namespace mclo
{
	template <typename T>
	concept arithmetic = std::is_arithmetic_v<T>;
}
