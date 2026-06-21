#pragma once

#ifdef __cpp_static_call_operator
#define MCLO_DETAIL_STATIC_CALL_OPERATOR static
#define MCLO_DETAIL_CONST_CALL_OPERATOR
#else
#define MCLO_DETAIL_STATIC_CALL_OPERATOR
#define MCLO_DETAIL_CONST_CALL_OPERATOR const
#endif

/// @brief Expands to @c static when the compiler supports a static @c operator(), otherwise nothing.
/// @details Pair with @ref MCLO_CONST_CALL_OPERATOR to declare a call operator that is @c static where supported
/// and @c const otherwise, giving the best available semantics for stateless function objects.
#define MCLO_STATIC_CALL_OPERATOR MCLO_DETAIL_STATIC_CALL_OPERATOR

/// @brief Expands to nothing when the compiler supports a static @c operator(), otherwise @c const.
#define MCLO_CONST_CALL_OPERATOR MCLO_DETAIL_CONST_CALL_OPERATOR
