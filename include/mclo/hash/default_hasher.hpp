#pragma once

#include "mclo/preprocessor/expand.hpp"
#include "mclo/preprocessor/stringify.hpp"

#ifndef MCLO_CONFIG_DEFAULT_HASHER
#define MCLO_CONFIG_DEFAULT_HASHER fnv1a_hasher
#endif

#include MCLO_STRINGIFY( MCLO_EXPAND( MCLO_CONFIG_DEFAULT_HASHER ).hpp )

namespace mclo
{
	using default_hasher = MCLO_CONFIG_DEFAULT_HASHER;
}
