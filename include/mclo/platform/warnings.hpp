#pragma once

#include "mclo/platform/compiler_detection.hpp"
#include "mclo/preprocessor/stringify.hpp"

#ifdef MCLO_COMPILER_MSVC
#define MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( ... )                                                                     \
	_Pragma( "warning( push )" ) _Pragma( MCLO_STRINGIFY( warning( disable : __VA_ARGS__ ) ) )

#define MCLO_MSVC_POP_WARNINGS _Pragma( "warning( pop )" )
#define MCLO_MSVC_PUSH_WARNING_LEVEL( LEVEL ) _Pragma( MCLO_STRINGIFY( warning( push, LEVEL ) ) )

#else
#define MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( ... )
#define MCLO_MSVC_POP_WARNINGS
#define MCLO_MSVC_PUSH_WARNING_LEVEL( LEVEL )
#endif

#ifdef MCLO_COMPILER_GCC_COMPATIBLE
#define MCLO_GCC_PUSH_AND_DISABLE_WARNINGS( ... )                                                                      \
	_Pragma( "GCC diagnostic push" ) _Pragma( MCLO_STRINGIFY( GCC warning ignored##__VA_ARGS__ ) )

#define MCLO_GCC_POP_WARNINGS _Pragma( "GCC warning pop" )
#else
#define MCLO_GCC_PUSH_AND_DISABLE_WARNINGS( ... )
#define MCLO_GCC_POP_WARNINGS
#endif
