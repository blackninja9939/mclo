#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_GCC_COMPATIBLE
#define MCLO_DETAIL_PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined( MCLO_COMPILER_MSVC )
#define MCLO_DETAIL_PRETTY_FUNCTION __FUNCSIG__
#else
#define MCLO_DETAIL_PRETTY_FUNCTION __func__
#endif

/// @brief A macro that expands to a string literal containing the name of the current function, including its signature
/// if supported by the compiler.
#define MCLO_PRETTY_FUNCTION MCLO_DETAIL_PRETTY_FUNCTION
