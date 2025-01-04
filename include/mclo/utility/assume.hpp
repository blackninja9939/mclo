#pragma once

#include <cassert>

#include "mclo/preprocessor/platform.hpp"

#if __has_cpp_attribute( assume )
#define MCLO_ASSUME( CONDITION ) [[assume( CONDITION )]]
#elif defined( _MSC_VER ) && !defined( __clang__ ) // MSVC
#define MCLO_ASSUME( CONDITION ) __assume( CONDITION )
#elif MCLO_HAS_BUILTIN( __builtin_assume ) // GCC, Clang & Others
#define MCLO_ASSUME( CONDITION ) __builtin_assume( CONDITION )
#else
#error Unspported compiler for assume expression
#endif

#ifdef NDEBUG
#define MCLO_ASSERT_ASSUME( CONDITION ) MCLO_ASSUME( CONDITION )
#else
#define MCLO_ASSERT_ASSUME( CONDITION ) assert( CONDITION )
#endif
