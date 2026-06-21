#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_MSVC
#define MCLO_DETAIL_EMPTY_BASES __declspec( empty_bases )
#define MCLO_DETAIL_RESTRICT __restrict
#define MCLO_DETAIL_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#define MCLO_DETAIL_NO_VTABLE __declspec( novtable )
#define MCLO_DETAIL_FORCE_INLINE __forceinline
#else
#define MCLO_DETAIL_EMPTY_BASES
#define MCLO_DETAIL_RESTRICT __restrict__
#define MCLO_DETAIL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#define MCLO_DETAIL_NO_VTABLE
#ifdef MCLO_COMPILER_GCC_COMPATIBLE
#define MCLO_DETAIL_FORCE_INLINE inline [[gnu::always_inline]]
#else
#define MCLO_DETAIL_FORCE_INLINE inline
#endif
#endif

/// @brief Applies empty base optimization to a class with multiple empty bases (MSVC @c __declspec(empty_bases)).
#define MCLO_EMPTY_BASES MCLO_DETAIL_EMPTY_BASES

/// @brief Marks a pointer as not aliasing any other pointer in scope, enabling further optimization.
#define MCLO_RESTRICT MCLO_DETAIL_RESTRICT

/// @brief Allows an empty member to occupy no space, even when it would normally require a unique address.
#define MCLO_NO_UNIQUE_ADDRESS MCLO_DETAIL_NO_UNIQUE_ADDRESS

/// @brief Suppresses generation of a vtable for a class, for abstract interfaces never instantiated directly.
#define MCLO_NO_VTABLE MCLO_DETAIL_NO_VTABLE

/// @brief Strongly requests that a function be inlined regardless of the compiler's heuristics.
#define MCLO_FORCE_INLINE MCLO_DETAIL_FORCE_INLINE
