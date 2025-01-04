#pragma once

#include "mclo/preprocessor/empty.hpp"
#include "mclo/preprocessor/if.hpp"
#include "mclo/preprocessor/is_defined.hpp"

#define MCLO_DEPRECATED_ALWAYS( MSG ) [[deprecated( MSG )]]

#define MCLO_DEPRECATED( CATEGORY, MSG )                                                                               \
	MCLO_IF( MCLO_IS_DEFINED( CATEGORY ) )( MCLO_DEPRECATED_ALWAYS( MSG ), MCLO_EMPTY )
