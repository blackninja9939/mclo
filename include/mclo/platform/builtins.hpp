#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef __has_builtin
#define MCLO_DETAIL_HAS_BUILTIN( X ) __has_builtin( X )
#else
#define MCLO_DETAIL_HAS_BUILTIN( X ) 0
#endif

/// @brief Evaluates to a non-zero value if the compiler provides the builtin @p X, otherwise 0.
/// @param X The name of the builtin to query, for example @c __builtin_expect.
#define MCLO_HAS_BUILTIN( X ) MCLO_DETAIL_HAS_BUILTIN( X )

#ifdef MCLO_COMPILER_MSVC
#define MCLO_DETAIL_MSVC_OR_HAS_BUILTIN( X ) 1
#else
#define MCLO_DETAIL_MSVC_OR_HAS_BUILTIN( X ) MCLO_HAS_BUILTIN( X )
#endif

/// @brief Evaluates to 1 on MSVC, otherwise forwards to @ref MCLO_HAS_BUILTIN for the builtin @p X.
/// @details MSVC does not implement @c __has_builtin but provides many builtins unconditionally, so this macro
/// treats MSVC as always having the queried builtin. Use it for builtins that MSVC is known to support.
/// @param X The name of the builtin to query.
#define MCLO_MSVC_OR_HAS_BUILTIN( X ) MCLO_DETAIL_MSVC_OR_HAS_BUILTIN( X )
