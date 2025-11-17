#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_MSVC
#include <__msvc_int128.hpp>
#endif

namespace mclo
{
#ifdef MCLO_COMPILER_MSVC
	using int128_t = std::_Signed128;
	using uint128_t = std::_Unsigned128;
#else
	using int128_t = __int128;
	using uint128_t = unsigned __int128;
#endif
}
