#pragma once

#define MCLO_STRINGIFY_IMPL( X ) #X

/// @brief Expands @p X and converts the result into a string literal.
/// @details The indirection through @c MCLO_STRINGIFY_IMPL ensures @p X is fully macro expanded before the @c #
/// stringization operator is applied, unlike using @c # directly which would stringize the unexpanded argument.
/// @param X The token to expand and stringify.
#define MCLO_STRINGIFY( X ) MCLO_STRINGIFY_IMPL( X )
