#pragma once

#include "mclo/random/random_generator.hpp"

// The default engine type is selected by the MCLO_CONFIG_DEFAULT_RANDOM_ENGINE macro and its declaring header by the
// MCLO_CONFIG_DEFAULT_RANDOM_ENGINE_HEADER macro. Both are supplied by the build system (see the
// MCLO_DEFAULT_RANDOM_ENGINE and MCLO_DEFAULT_RANDOM_ENGINE_HEADER CMake cache variables) and may name an engine that
// is not part of mclo, allowing a project to plug in its own implementation.
//
// These macros must be defined consistently for every translation unit in the build, otherwise
// default_random_generator resolves to different types across translation units and violates the One Definition
// Rule. Configure them through the CMake cache variables rather than per-file #define directives.

#ifndef MCLO_CONFIG_DEFAULT_RANDOM_ENGINE
#error                                                                                                                 \
	"MCLO_CONFIG_DEFAULT_RANDOM_ENGINE is not defined. Configure it via the MCLO_DEFAULT_RANDOM_ENGINE CMake cache variable."
#endif

#ifndef MCLO_CONFIG_DEFAULT_RANDOM_ENGINE_HEADER
#error                                                                                                                 \
	"MCLO_CONFIG_DEFAULT_RANDOM_ENGINE_HEADER is not defined. Configure it via the MCLO_DEFAULT_RANDOM_ENGINE_HEADER CMake cache variable."
#endif

#include MCLO_CONFIG_DEFAULT_RANDOM_ENGINE_HEADER

namespace mclo
{
	/// @brief A @ref random_generator instantiated with the library's default random engine.
	/// @see MCLO_CONFIG_DEFAULT_RANDOM_ENGINE
	using default_random_generator = random_generator<MCLO_CONFIG_DEFAULT_RANDOM_ENGINE>;
}
