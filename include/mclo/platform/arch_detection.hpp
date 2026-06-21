#pragma once

// Detects the target CPU architecture from the predefined macros supplied by the compiler and defines exactly one
// MCLO_ARCH_* macro. If the architecture cannot be determined a hard compile error is raised so unsupported targets
// fail loudly rather than silently misbehaving.

#if defined( _M_IX86 ) || defined( _M_X64 ) || defined( __i386__ ) || defined( __x86_64__ )
/// @brief Defined when compiling for a 32 or 64 bit x86 architecture.
#define MCLO_ARCH_X86
#elif defined( __arm__ ) || defined( __aarch64__ ) || defined( _M_ARM ) || defined( _M_ARM64 )
/// @brief Defined when compiling for a 32 or 64 bit ARM architecture.
#define MCLO_ARCH_ARM
#elif defined( __powerpc__ ) || defined( __ppc__ ) || defined( __PPC__ )
/// @brief Defined when compiling for a PowerPC architecture.
#define MCLO_ARCH_POWERPC
#elif defined( __riscv )
/// @brief Defined when compiling for a RISC-V architecture.
#define MCLO_ARCH_RISCV
#else
#error Unable to detect architecture
#endif
