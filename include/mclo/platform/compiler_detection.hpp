#pragma once

#if defined( __clang__ )
#define MCLO_COMPILER_CLANG
#define MCLO_COMPILER_GCC_COMPATIBLE
#elif defined( __GNUC__ )
#define MCLO_COMPILER_GCC
#define MCLO_COMPILER_GCC_COMPATIBLE
#elif defined( _MSC_VER )
#define MCLO_COMPILER_MSVC
#else
#error Unable to detect compiler
#endif
