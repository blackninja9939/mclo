#pragma once

#define MCLO_CONCAT_IMPL( A, ... ) A##__VA_ARGS__

/// @brief Concatenates @p A with the remaining arguments into a single token, expanding each first.
/// @details The indirection through @c MCLO_CONCAT_IMPL ensures that macro arguments are fully expanded before the
/// @c ## token pasting occurs, unlike using @c ## directly. Trailing arguments are joined with @c __VA_ARGS__ so
/// commas within them are preserved.
/// @param A The leading token to paste onto.
/// @param ... The trailing token(s) to paste after @p A.
#define MCLO_CONCAT( A, ... ) MCLO_CONCAT_IMPL( A, __VA_ARGS__ )
