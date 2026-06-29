#pragma once

#include "mclo/platform/compiler_detection.hpp"
#include "mclo/preprocessor/stringify.hpp"

// MCLO_DISABLE_WARNINGS pushes the current warning state and disables each named warning passed to it; pair it with
// MCLO_RESTORE_WARNINGS. Pass a space separated list of MCLO_WARNING_* macros. Each names a single warning and lists
// every platform's spelling in one place, expanding only the directive for the active compiler.

#ifdef MCLO_COMPILER_MSVC

/// @brief Push the current warning state and disable the given named warnings for the following code.
/// @param ... A space separated list of @c MCLO_WARNING_* macros naming the warnings to disable.
#define MCLO_DISABLE_WARNINGS( ... ) _Pragma( "warning( push )" ) __VA_ARGS__

/// @brief Restore the warning state saved by the matching @c MCLO_DISABLE_WARNINGS.
#define MCLO_RESTORE_WARNINGS _Pragma( "warning( pop )" )

#define MCLO_DETAIL_MSVC_DISABLE_WARNING( CODE ) _Pragma( MCLO_STRINGIFY( warning( disable : CODE ) ) )

#elif defined( MCLO_COMPILER_GCC_COMPATIBLE )

/// @brief Push the current warning state and disable the given named warnings for the following code.
/// @param ... A space separated list of @c MCLO_WARNING_* macros naming the warnings to disable.
#define MCLO_DISABLE_WARNINGS( ... ) _Pragma( "GCC diagnostic push" ) __VA_ARGS__

/// @brief Restore the warning state saved by the matching @c MCLO_DISABLE_WARNINGS.
#define MCLO_RESTORE_WARNINGS _Pragma( "GCC diagnostic pop" )

#define MCLO_DETAIL_GCC_DISABLE_WARNING( NAME ) _Pragma( MCLO_STRINGIFY( GCC diagnostic ignored NAME ) )

#else

#define MCLO_DISABLE_WARNINGS( ... )
#define MCLO_RESTORE_WARNINGS

#endif

// No-op fallbacks so a named warning can list directives for every platform; only the active compiler's one expands.
#ifndef MCLO_DETAIL_MSVC_DISABLE_WARNING
#define MCLO_DETAIL_MSVC_DISABLE_WARNING( CODE )
#endif
#ifndef MCLO_DETAIL_GCC_DISABLE_WARNING
#define MCLO_DETAIL_GCC_DISABLE_WARNING( NAME )
#endif

// Named warnings, pass these to MCLO_DISABLE_WARNINGS. Defined once with each platform's spelling side by side.

/// @brief A structure was padded due to an alignment specifier.
#define MCLO_WARNING_ALIGNMENT_PADDING                                                                                 \
	MCLO_DETAIL_MSVC_DISABLE_WARNING( 4324 ) MCLO_DETAIL_GCC_DISABLE_WARNING( "-Wpadded" )

/// @brief A member variable is not initialized by a constructor.
#define MCLO_WARNING_UNINITIALIZED_MEMBER MCLO_DETAIL_MSVC_DISABLE_WARNING( 26495 )

/// @brief An integral constant expression overflowed.
#define MCLO_WARNING_CONSTANT_OVERFLOW                                                                                 \
	MCLO_DETAIL_MSVC_DISABLE_WARNING( 4307 ) MCLO_DETAIL_GCC_DISABLE_WARNING( "-Woverflow" )

/// @brief A cast truncates a constant value.
#define MCLO_WARNING_CONSTANT_TRUNCATION MCLO_DETAIL_MSVC_DISABLE_WARNING( 4310 )

/// @brief An arithmetic operation overflows.
#define MCLO_WARNING_ARITHMETIC_OVERFLOW MCLO_DETAIL_MSVC_DISABLE_WARNING( 26450 )

/// @brief Using a moved from object, which is in a valid but unspecified state.
#define MCLO_WARNING_USING_MOVED_FROM_OBJECT MCLO_DETAIL_MSVC_DISABLE_WARNING( 26800 )
