#pragma once

#if defined( _M_IX86 ) || defined( _M_X64 ) || defined( __i386__ ) || defined( __x86_64__ )
#define MCLO_ARCH_X86
#elif defined( __arm__ ) || defined( __aarch64__ ) || defined( _M_ARM ) || defined( _M_ARM64 )
#define MCLO_ARCH_ARM
#elif defined( __powerpc__ ) || defined( __ppc__ ) || defined( __PPC__ )
#define MCLO_ARCH_POWERPC
#elif defined( __riscv )
#define MCLO_ARCH_RISCV
#else
#error Unable to detect architecture
#endif
