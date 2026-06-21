#pragma once

// Detects the compiler in use and defines exactly one of MCLO_COMPILER_CLANG, MCLO_COMPILER_GCC or
// MCLO_COMPILER_MSVC. MCLO_COMPILER_GCC_COMPATIBLE is additionally defined for both Clang and GCC, so code relying
// on GCC style extensions can target both with a single check. If the compiler cannot be determined a hard compile
// error is raised.

#if defined( __clang__ )
/// @brief Defined when compiling with Clang.
#define MCLO_COMPILER_CLANG
/// @brief Defined when compiling with a GCC compatible compiler (Clang or GCC).
#define MCLO_COMPILER_GCC_COMPATIBLE
#elif defined( __GNUC__ )
/// @brief Defined when compiling with GCC.
#define MCLO_COMPILER_GCC
/// @brief Defined when compiling with a GCC compatible compiler (Clang or GCC).
#define MCLO_COMPILER_GCC_COMPATIBLE
#elif defined( _MSC_VER )
/// @brief Defined when compiling with MSVC.
#define MCLO_COMPILER_MSVC
#else
#error Unable to detect compiler
#endif
