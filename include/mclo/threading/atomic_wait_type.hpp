#pragma once

#include "mclo/platform/os_detection.hpp"

namespace mclo
{
#ifdef MCLO_OS_WINDOWS
	// On Windows any atomic type supports intrinsic wait via WaitOnAddress
	using atomic_wait_type = unsigned char;
#else
	// On basically all other platforms it needs to be 32 bits, eg: Futex, ulock, etc.
	// Some also support 64 bits but 32 is the most widely supported and smaller anyway.
	using atomic_wait_type = unsigned int;
#endif
}
