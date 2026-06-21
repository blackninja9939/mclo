#pragma once

// todo(mc) this should be something we can set in CMake
// keeps DEBUG_ASSERT actually as a debug assert in all tests etc
#ifdef MCLO_CONFIG_ENABLE_TESTING
#undef NDEBUG
#endif

#include <libassert/assert.hpp>

// In test builds assertion macros throw exceptions so they can be caught and validated, which would terminate a
// noexcept function. A function that should be noexcept but contains assertions uses these macros instead so the
// noexcept specifier is dropped in test builds, allowing the exception to propagate.

#ifdef MCLO_CONFIG_ENABLE_TESTING
#define MCLO_DETAIL_NOEXCEPT_TESTS
#define MCLO_DETAIL_NOEXCEPT_TESTS_IF( ... )
#else
#define MCLO_DETAIL_NOEXCEPT_TESTS noexcept
#define MCLO_DETAIL_NOEXCEPT_TESTS_IF( ... ) noexcept( __VA_ARGS__ )
#endif

/// @brief Expands to noexcept in normal builds and nothing in test builds
/// @details Use in place of noexcept on functions whose body contains assertion macros
#define MCLO_NOEXCEPT_TESTS MCLO_DETAIL_NOEXCEPT_TESTS

/// @brief Expands to noexcept( ... ) in normal builds and nothing in test builds
/// @details Use in place of noexcept( ... ) on functions whose body contains assertion macros
#define MCLO_NOEXCEPT_TESTS_IF( ... ) MCLO_DETAIL_NOEXCEPT_TESTS_IF( __VA_ARGS__ )
