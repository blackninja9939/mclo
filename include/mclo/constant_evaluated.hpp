#pragma once

#include <type_traits>

#include "platform.hpp"

namespace mclo
{
#ifdef __cpp_lib_is_constant_evaluated
	using std::is_constant_evaluated;
#else
	[[nodiscard]] constexpr bool is_constant_evaluated() noexcept
	{
#if MCLO_MSVC_OR_HAS_BUILTIN( __builtin_is_constant_evaluated )
		return __builtin_is_constant_evaluated();
#else
#error Unsupported platform for constant evaluation check
#endif
	}
#endif
}
