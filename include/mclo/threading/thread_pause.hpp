#pragma once

#include "mclo/platform/arch_detection.hpp"
#include "mclo/platform/attributes.hpp"
#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_MSVC
#include <immintrin.h>
#endif

namespace mclo
{
	/// @brief Emits a CPU pause/yield hint to improve the efficiency of busy-wait spin loops.
	/// @details Issues the architecture-appropriate spin-loop hint instruction (e.g. x86 @c PAUSE, ARM @c YIELD).
	/// This is a lightweight CPU-level hint that reduces power consumption and improves performance on hyper-threaded
	/// cores while spinning. It is not a thread yield and does not relinquish the time slice to the scheduler.
	/// @note Falls back to a @c nop, or no operation at all, on architectures without a dedicated spin hint.
	MCLO_FORCE_INLINE void thread_pause() noexcept
	{
#ifdef MCLO_ARCH_X86
#ifdef MCLO_COMPILER_MSVC
		_mm_pause(); // x86 PAUSE instruction via intrinsic
#elif defined( MCLO_COMPILER_GCC_COMPATIBLE )
		__builtin_ia32_pause(); // x86 PAUSE instruction via built in
#endif
#elif defined( MCLO_ARCH_ARM ) && defined( MCLO_COMPILER_GCC_COMPATIBLE )
		__builtin_arm_yield(); // ARM yield instruction, not the same as a thread yield, is much lighter weight
#elif defined( MCLO_ARCH_POWERPC ) && defined( MCLO_COMPILER_GCC_COMPATIBLE )
		__asm__ __volatile__( "or 27,27,27" ); // PPC spin hint
#elif defined( MCLO_COMPILER_GCC_COMPATIBLE )
		__asm__ __volatile__( "nop" ); // Fall back to nop if inline asm is supported
#endif
	}
}
