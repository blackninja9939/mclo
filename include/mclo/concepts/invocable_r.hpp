#pragma once

#include <concepts>
#include <type_traits>

namespace mclo
{
	/// @brief Requires that F is invocable with Args... and its result is convertible to R
	template <typename R, typename F, typename... Args>
	concept invocable_r = std::invocable<F, Args...> && std::convertible_to<std::invoke_result_t<F, Args...>, R>;
}
