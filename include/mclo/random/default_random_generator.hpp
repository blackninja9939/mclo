#pragma once

#include "mclo/preprocessor/expand.hpp"
#include "mclo/preprocessor/stringify.hpp"
#include "mclo/random/random_generator.hpp"

#ifndef MCLO_CONFIG_DEFAULT_RANDOM_ENGINE
#define MCLO_CONFIG_DEFAULT_RANDOM_ENGINE xoshiro256plusplus
#endif

#include MCLO_STRINGIFY( MCLO_EXPAND( MCLO_CONFIG_DEFAULT_RANDOM_ENGINE ).hpp )

namespace mclo
{
	using default_random_generator = random_generator<MCLO_CONFIG_DEFAULT_RANDOM_ENGINE>;
}
