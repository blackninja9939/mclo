#pragma once

namespace mclo
{
	/// @brief The smallest atomic integer type guaranteed to support the platform's intrinsic wait/wake primitives.
	/// @details On Windows any atomic integer type supports intrinsic waiting via @c WaitOnAddress, but on most other
	/// platforms the underlying primitive (e.g. Futex or ulock) requires a 32-bit type. Some platforms additionally
	/// support 64-bit waits, but 32-bit is the most widely supported and is smaller anyway, so it is chosen as the
	/// portable default.
	using atomic_wait_type = unsigned int;
}
