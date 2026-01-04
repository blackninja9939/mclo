#pragma once

#include "mclo/platform/arch_detection.hpp"
#include "mclo/platform/compiler_detection.hpp"
#include "mclo/preprocessor/platform.hpp"

#ifdef MCLO_COMPILER_MSVC
#include <immintrin.h>
#endif

namespace mclo
{
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
