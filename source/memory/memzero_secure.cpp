// Opt in to the C11 Annex K bounds-checked functions (memset_s) where the C library provides them. This must be
// defined before any standard library header is included.
#ifndef __STDC_WANT_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "mclo/memory/memzero_secure.hpp"

#include "mclo/platform/compiler_detection.hpp"
#include "mclo/platform/os_detection.hpp"
#include "mclo/platform/windows_wrapper.hpp"

#include <algorithm>
#include <cstring>

#if defined( MCLO_OS_LINUX ) || defined( MCLO_OS_ANDROID ) || defined( MCLO_OS_APPLE )
#include <string.h> // explicit_bzero
#endif

void mclo::memzero_secure( void* const ptr, const std::size_t size ) noexcept
{
// std::memset_explicit was added in C++26 but has no feature-test macro, so key off the language version.
#if __cplusplus > 202302L
	std::memset_explicit( ptr, 0, size );
#elif defined( __STDC_LIB_EXT1__ )
	::memset_s( ptr, size, 0, size );
#elif defined( MCLO_OS_WINDOWS )
	SecureZeroMemory( ptr, size );
#elif defined( MCLO_OS_LINUX ) || defined( MCLO_OS_ANDROID ) || defined( MCLO_OS_APPLE )
	::explicit_bzero( ptr, size );
#elif defined( MCLO_COMPILER_GCC_COMPATIBLE )
	// An empty asm block with a memory clobber forces the preceding store to be observable, preventing the optimiser
	// from removing it as a dead store.
	std::memset( ptr, 0, size );
	__asm__ __volatile__( "" : : "r"( ptr ) : "memory" );
#else
	std::fill_n( static_cast<volatile unsigned char*>( ptr ), size, static_cast<unsigned char>( 0 ) );
#endif
}
