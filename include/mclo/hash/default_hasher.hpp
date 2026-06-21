#pragma once

#include "mclo/hash/hasher.hpp"

// The default hasher type is selected by the MCLO_CONFIG_DEFAULT_HASHER macro and its declaring header by the
// MCLO_CONFIG_DEFAULT_HASHER_HEADER macro. Both are supplied by the build system (see the MCLO_DEFAULT_HASHER and
// MCLO_DEFAULT_HASHER_HEADER CMake cache variables) and may name a hasher that is not part of mclo, allowing a
// project to plug in its own implementation.
//
// These macros must be defined consistently for every translation unit in the build, otherwise default_hasher
// resolves to different types across translation units and violates the One Definition Rule. Configure them through
// the CMake cache variables rather than per-file #define directives.

#ifndef MCLO_CONFIG_DEFAULT_HASHER
#error "MCLO_CONFIG_DEFAULT_HASHER is not defined. Configure it via the MCLO_DEFAULT_HASHER CMake cache variable."
#endif

#ifndef MCLO_CONFIG_DEFAULT_HASHER_HEADER
#error                                                                                                                 \
	"MCLO_CONFIG_DEFAULT_HASHER_HEADER is not defined. Configure it via the MCLO_DEFAULT_HASHER_HEADER CMake cache variable."
#endif

#include MCLO_CONFIG_DEFAULT_HASHER_HEADER

namespace mclo
{
	/// @brief The library's default hasher type, selected by @c MCLO_CONFIG_DEFAULT_HASHER.
	/// @see MCLO_CONFIG_DEFAULT_HASHER
	using default_hasher = MCLO_CONFIG_DEFAULT_HASHER;

	static_assert( hasher<default_hasher>,
				   "MCLO_CONFIG_DEFAULT_HASHER must name a type satisfying the mclo::hasher concept" );
}
