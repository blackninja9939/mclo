#pragma once

#include "mclo/preprocessor/stringify.hpp"

#ifdef _MSC_VER
#define MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( ... )                                                                     \
	_Pragma( "warning( push )" ) _Pragma( MCLO_STRINGIFY( warning( disable : __VA_ARGS__ ) ) )

#define MCLO_MSVC_POP_WARNINGS _Pragma( "warning( pop )" )
#define MCLO_MSVC_PUSH_WARNING_LEVEL( LEVEL ) _Pragma( MCLO_STRINGIFY( warning( push, LEVEL ) ) )

#else
#define MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( ... )
#define MCLO_MSVC_POP_WARNINGS
#define MCLO_MSVC_PUSH_WARNING_LEVEL( LEVEL )
#endif

#ifdef __GNUC__
#define MCLO_GCC_PUSH_AND_DISABLE_WARNINGS( ... )                                                                      \
	_Pragma( "GCC diagnostic push" ) _Pragma( MCLO_STRINGIFY( GCC warning ignored##__VA_ARGS__ ) )

#define MCLO_GCC_POP_WARNINGS _Pragma( "GCC warning pop" )
#else
#define MCLO_GCC_PUSH_AND_DISABLE_WARNINGS( ... )
#define MCLO_GCC_POP_WARNINGS
#endif

#ifdef __has_builtin
#define MCLO_HAS_BUILTIN( X ) __has_builtin( X )
#else
#define MCLO_HAS_BUILTIN( X ) 0
#endif

#ifdef _MSC_VER
#define MCLO_MSVC_OR_HAS_BUILTIN( X ) 1
#else
#define MCLO_MSVC_OR_HAS_BUILTIN( X ) MCLO_HAS_BUILTIN( X )
#endif

#ifdef _MSC_VER
#define MCLO_EMPTY_BASES __declspec( empty_bases )
#else
#define MCLO_EMPTY_BASES
#endif

#ifdef _MSC_VER
#define MCLO_RESTRICT __restrict
#else
#define MCLO_RESTRICT __restrict__
#endif

#ifdef __cpp_static_call_operator
#define MCLO_STATIC_CALL_OPERATOR static
#define MCLO_CONST_CALL_OPERATOR
#else
#define MCLO_STATIC_CALL_OPERATOR
#define MCLO_CONST_CALL_OPERATOR const
#endif

#define MCLO_NOEXCEPT_AND_BODY( EXPRESSION )                                                                           \
	noexcept( noexcept( EXPRESSION ) )                                                                                 \
	{                                                                                                                  \
		return EXPRESSION;                                                                                             \
	}

#define MCLO_NO_UNIQUE_ADDRESS [[no_unique_address]] [[msvc::no_unique_address]]

#ifdef _MSC_VER
#define MCLO_NO_VTABLE __declspec( novtable )
#else
#define MCLO_NO_VTABLE
#endif
