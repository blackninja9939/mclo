#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef __has_builtin
#define MCLO_HAS_BUILTIN( X ) __has_builtin( X )
#else
#define MCLO_HAS_BUILTIN( X ) 0
#endif

#ifdef MCLO_COMPILER_MSVC
#define MCLO_MSVC_OR_HAS_BUILTIN( X ) 1
#else
#define MCLO_MSVC_OR_HAS_BUILTIN( X ) MCLO_HAS_BUILTIN( X )
#endif
