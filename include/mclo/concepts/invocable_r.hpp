#pragma once

#include <concepts>
#include <type_traits>

namespace mclo
{
	template <typename R, typename F, typename... Args>
	concept invocable_r = std::invocable<F, Args...> && std::convertible_to<std::invoke_result_t<F, Args...>, R>;
}
