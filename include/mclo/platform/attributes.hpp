#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_MSVC
#define MCLO_EMPTY_BASES __declspec( empty_bases )
#else
#define MCLO_EMPTY_BASES
#endif

#ifdef MCLO_COMPILER_MSVC
#define MCLO_RESTRICT __restrict
#else
#define MCLO_RESTRICT __restrict__
#endif

#ifdef MCLO_COMPILER_MSVC
#define MCLO_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define MCLO_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#ifdef MCLO_COMPILER_MSVC
#define MCLO_NO_VTABLE __declspec( novtable )
#else
#define MCLO_NO_VTABLE
#endif

#ifdef MCLO_COMPILER_MSVC
#define MCLO_FORCE_INLINE __forceinline
#elif defined( MCLO_COMPILER_GCC_COMPATIBLE )
#define MCLO_FORCE_INLINE inline [[gnu::always_inline]]
#else
#define MCLO_FORCE_INLINE inline
#endif
