#pragma once

#include "mclo/hash/fnv1a_hasher.hpp"

namespace mclo
{
#ifdef MCLO_CONFIG_DEFAULT_HASHER
	using default_hasher = MCLO_CONFIG_DEFAULT_HASHER;
#else
	using default_hasher = fnv1a_hasher;
#endif
}
