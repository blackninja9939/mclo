#pragma once

/// @brief Expands to @p X, triggering an extra rescan of the argument.
/// @details Primarily used to work around MSVC's non conforming handling of @c __VA_ARGS__, where wrapping a macro
/// invocation in @ref MCLO_EXPAND forces the variadic arguments to be expanded before being passed on.
/// @param X The tokens to expand.
#define MCLO_EXPAND( X ) X
