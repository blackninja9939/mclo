#pragma once

#define MCLO_ARG_PLACEHOLDER_1 0,
#define MCLO_TAKE_SECOND_ARG( IGNORE, VALUE, ... ) VALUE

/// @brief Expands to @c 1 if the macro @p X is defined (to nothing or to @c 1), otherwise @c 0.
/// @details Implements the classic "config macro" detection trick: a macro @c X defined as @c 1 causes the
/// internal @c MCLO_ARG_PLACEHOLDER_##X token to expand and inject an extra argument, which shifts the selected
/// argument from @c 0 to @c 1. Pair with @ref MCLO_IF to conditionally enable code based on a feature flag macro.
/// @param X The macro name to test for definition.
#define MCLO_IS_DEFINED( X ) MCLO_IS_DEFINED_IMPL( X )
#define MCLO_IS_DEFINED_IMPL( X ) MCLO_IS_DEFINED_IMPL_IMPL( MCLO_ARG_PLACEHOLDER_##X )
#define MCLO_IS_DEFINED_IMPL_IMPL( ARG1_OR_JUNK ) MCLO_TAKE_SECOND_ARG( ARG1_OR_JUNK 1, 0 )
