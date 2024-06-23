#pragma once

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
