#pragma once

#include "mclo/platform/compiler_detection.hpp"

namespace mclo
{
	[[noreturn]] inline void unreachable()
	{
		// Even if no compiler specific extension is used, undefined behavior is still raised by
		// the noreturn attribute, so any subsequent code is considered undefined behavior.
#ifdef MCLO_COMPILER_GCC_COMPATIBLE
		__builtin_unreachable();
#elif defined( MCLO_COMPILER_MSVC )
		__assume( false );
#endif
	}
}
