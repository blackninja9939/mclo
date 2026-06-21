#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_MSVC
#define MCLO_DETAIL_ALLOCA_RAW _alloca
#else
#define MCLO_DETAIL_ALLOCA_RAW alloca
#endif

/// @brief Allocates the given number of bytes of uninitialized memory on the calling function's stack.
/// @details Wraps the compiler-specific stack allocation intrinsic (@c _alloca on MSVC, @c alloca otherwise). The
/// memory is automatically released when the calling function returns.
/// @warning The allocation is bound to the enclosing @b function, not the enclosing scope. It is not released when a
/// block, loop iteration, or other inner scope ends, only when the function returns. Calling this inside a loop
/// therefore accumulates a separate allocation per iteration and can quickly overflow the stack; never allocate in a
/// loop.
/// @warning The memory is only valid until the enclosing function returns and must not be returned or stored beyond
/// it. Allocating a large or unbounded amount will overflow the stack; use only for small, bounded sizes.
#define MCLO_ALLOCA_RAW MCLO_DETAIL_ALLOCA_RAW

/// @brief Allocates stack space for @p AMOUNT objects of type @p TYPE and yields a @c TYPE* to it.
/// @details Allocates @c sizeof(TYPE)*AMOUNT bytes via @ref MCLO_ALLOCA_RAW and reinterpret_casts the result. The
/// memory is uninitialized; no objects are constructed in it.
/// @param TYPE The element type to reserve space for.
/// @param AMOUNT The number of elements to reserve space for.
/// @warning The returned pointer is only valid until the enclosing function returns; see @ref MCLO_ALLOCA_RAW.
#define MCLO_ALLOCA_TYPED( TYPE, AMOUNT ) reinterpret_cast<TYPE*>( MCLO_ALLOCA_RAW( sizeof( TYPE ) * AMOUNT ) )
