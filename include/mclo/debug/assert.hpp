#pragma once

#include <libassert/assert.hpp>

#ifdef MCLO_CONFIG_ENABLE_TESTING
#define MCLO_NOEXCEPT_TESTS
#define MCLO_NOEXCEPT_TESTS_IF( ... )
#else
#define MCLO_NOEXCEPT_TESTS noexcept
#define MCLO_NOEXCEPT_TESTS_IF( ... ) noexcept( __VA_ARGS__ )
#endif
