#pragma once

/// @brief Selects between two values based on a boolean @p CONDITION.
/// @details @p CONDITION must expand to either @c 0 or @c 1, for example via @ref MCLO_IS_DEFINED. The macro then
/// chooses the false or true branch respectively. Invoke the result with two arguments: the value for the true
/// branch and the value for the false branch, for example @c MCLO_IF( cond )( true_value, false_value ).
/// @param CONDITION A token expanding to @c 0 or @c 1.
#define MCLO_IF( CONDITION ) MCLO_CONCAT_IMPL( MCLO_IF_, CONDITION )
#define MCLO_IF_0( TRUE_VALUE, FALSE_VALUE ) FALSE_VALUE
#define MCLO_IF_1( TRUE_VALUE, FALSE_VALUE ) TRUE_VALUE
