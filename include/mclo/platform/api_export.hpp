#pragma once

#include "mclo/platform/compiler_detection.hpp"
#include "mclo/platform/os_detection.hpp"

/// @file api_export.hpp
/// @brief Cross platform macros for controlling symbol visibility in shared libraries.
/// @details Use @ref MCLO_API_EXPORT when building a shared library and @ref MCLO_API_IMPORT when consuming one.
/// The typical pattern is to define a per library macro that resolves to one or the other based on whether the
/// library is currently being built, for example:
/// @code
/// #ifdef MY_LIBRARY_BUILDING
/// #define MY_LIBRARY_API MCLO_API_EXPORT
/// #else
/// #define MY_LIBRARY_API MCLO_API_IMPORT
/// #endif
/// @endcode

#if defined( MCLO_OS_WINDOWS ) && defined( MCLO_COMPILER_MSVC )

/// @brief Marks a symbol as exported from a shared library.
#define MCLO_API_EXPORT __declspec( dllexport )

/// @brief Marks a symbol as imported from a shared library.
#define MCLO_API_IMPORT __declspec( dllimport )

/// @brief Marks a symbol as hidden, preventing it from being exported from a shared library.
#define MCLO_API_HIDDEN

#elif defined( MCLO_OS_WINDOWS ) && defined( MCLO_COMPILER_GCC_COMPATIBLE )

#define MCLO_API_EXPORT __attribute__( ( dllexport ) )
#define MCLO_API_IMPORT __attribute__( ( dllimport ) )
#define MCLO_API_HIDDEN

#elif defined( MCLO_COMPILER_GCC_COMPATIBLE )

#define MCLO_API_EXPORT __attribute__( ( visibility( "default" ) ) )
#define MCLO_API_IMPORT __attribute__( ( visibility( "default" ) ) )
#define MCLO_API_HIDDEN __attribute__( ( visibility( "hidden" ) ) )

#else

#define MCLO_API_EXPORT
#define MCLO_API_IMPORT
#define MCLO_API_HIDDEN

#endif
