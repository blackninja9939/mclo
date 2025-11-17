#pragma once

#include "mclo/platform/compiler_detection.hpp"

#ifdef MCLO_COMPILER_MSVC
#define MCLO_ALLOCA_RAW _alloca
#else
#define MCLO_ALLOCA_RAW alloca
#endif

#define MCLO_ALLOCA_TYPED( TYPE, AMOUNT ) reinterpret_cast<TYPE*>( MCLO_ALLOCA_RAW( sizeof( TYPE ) * AMOUNT ) )
