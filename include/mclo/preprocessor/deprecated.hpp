#pragma once

#include "mclo/preprocessor/empty.hpp"
#include "mclo/preprocessor/if.hpp"
#include "mclo/preprocessor/is_defined.hpp"

/// @brief Unconditionally marks an entity as deprecated with the given message.
/// @param MSG A string literal describing the deprecation and suggested replacement.
#define MCLO_DEPRECATED_ALWAYS( MSG ) [[deprecated( MSG )]]

/// @brief Marks an entity as deprecated only when the category flag macro is defined.
/// @details Expands to @c [[deprecated(MSG)]] when the macro named by @p CATEGORY is defined to @c 1, otherwise to
/// nothing. This lets deprecations be opted into per category, so a codebase can enable warnings for one group of
/// deprecated APIs at a time while migrating.
/// @param CATEGORY The name of a flag macro that enables the deprecation when defined.
/// @param MSG A string literal describing the deprecation and suggested replacement.
#define MCLO_DEPRECATED( CATEGORY, MSG )                                                                               \
	MCLO_IF( MCLO_IS_DEFINED( CATEGORY ) )( MCLO_DEPRECATED_ALWAYS( MSG ), MCLO_EMPTY )
