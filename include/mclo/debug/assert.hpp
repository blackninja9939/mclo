#pragma once

// Allow users to provide their own assert configuration header, which can override the default behaviour of the macros
// defined here. This is useful for projects that want to integrate with their own logging or error handling systems or
// for projects that want to disable assertions in certain build configurations.
#ifdef MCLO_ASSERT_CONFIG_HEADER
#include MCLO_ASSERT_CONFIG_HEADER
#endif

#include "mclo/debug/breakpoint.hpp"
#include "mclo/debug/unreachable.hpp"
#include "mclo/platform/compiler_detection.hpp"

#include <format>

namespace mclo::detail
{
	[[noreturn]] void assert_failed( const char* expression,
									 const char* file,
									 const unsigned line,
									 const char* message,
									 const char* formatted_args ) noexcept;

	// We breakpoint before calling the core assert_failed in case the assertion failure itself throws bad_alloc when
	// formatting the message. This allows the user to inspect the assertion in a debugger before the program aborts.

	[[noreturn]] inline void assert_failed( const char* expression, const char* file, unsigned line ) noexcept
	{
		mclo::breakpoint_if_debugging();
		assert_failed( expression, file, line, nullptr, nullptr );
	}

	[[noreturn]] inline void assert_failed( const char* expression,
											const char* file,
											unsigned line,
											const char* message ) noexcept
	{
		mclo::breakpoint_if_debugging();
		assert_failed( expression, file, line, message, nullptr );
	}

	template <typename T>
	void format_assert_arg( std::string& out, std::size_t index, const T& )
	{
		std::format_to( std::back_inserter( out ), "Argument {}: unknown\n", index );
	}

	template <typename T>
		requires requires { std::formatter<T>(); }
	void format_assert_arg( std::string& out, std::size_t index, const T& arg )
	{
		std::format_to( std::back_inserter( out ), "Argument {}: {}\n", index, arg );
	}

	template <typename... Args>
		requires( sizeof...( Args ) > 0 )
	[[noreturn]] void assert_failed(
		const char* expression, const char* file, unsigned line, const char* message, const Args&... args ) noexcept
	{
		mclo::breakpoint_if_debugging();

		std::string arg_string;
		arg_string.reserve( sizeof...( args ) * 24 );

		std::size_t index = 0;
		( format_assert_arg( arg_string, index++, args ), ... );

		assert_failed( expression, file, line, message, arg_string.c_str() );
	}
}

#ifndef MCLO_DEBUG_ASSERT
#ifdef NDEBUG
#define MCLO_DEBUG_ASSERT( EXPR, ... ) ( ( void )0 )
#else
#define MCLO_DEBUG_ASSERT( EXPR, ... )                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( EXPR ) ) [[unlikely]]                                                                                  \
		{                                                                                                              \
			mclo::detail::assert_failed( #EXPR, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__ ) );                       \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )
#endif
#endif

#ifndef MCLO_ASSERT
#define MCLO_ASSERT( EXPR, ... )                                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( EXPR ) ) [[unlikely]]                                                                                  \
		{                                                                                                              \
			mclo::detail::assert_failed( #EXPR, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__ ) );                       \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )
#endif

#ifndef MCLO_ASSUME
#ifdef NDEBUG
// Prefer the cross platform attribute when available, otherwise fall back to platform specific extensions
#if __has_cpp_attribute( assume )
#define MCLO_ASSUME( EXPR, ... ) [[assume( EXPR )]]
#elif defined( MCLO_COMPILER_CLANG )
#define MCLO_ASSUME( EXPR, ... ) __builtin_assume( EXPR )
#elif defined( MCLO_COMPILER_GCC )
#define MCLO_ASSUME( EXPR, ... ) __attribute__( ( assume( EXPR ) ) )
#elif defined( MCLO_COMPILER_MSVC )
#define MCLO_ASSUME( EXPR, ... ) __assume( EXPR )
#else
#define MCLO_ASSUME( EXPR, ... )
#endif
#else
#define MCLO_ASSUME( EXPR, ... )                                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( EXPR ) ) [[unlikely]]                                                                                  \
		{                                                                                                              \
			mclo::detail::assert_failed( #EXPR, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__ ) );                       \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )
#endif
#endif

#ifndef MCLO_PANIC
#define MCLO_PANIC( ... )                                                                                              \
	do                                                                                                                 \
	{                                                                                                                  \
		mclo::detail::assert_failed( "PANIC", __FILE__, __LINE__, __VA_ARGS__ );                                       \
	}                                                                                                                  \
	while ( 0 )
#endif

#ifndef MCLO_UNREACHABLE
#ifdef NDEBUG
#define MCLO_UNREACHABLE( ... ) mclo::unreachable()
#else
#define MCLO_UNREACHABLE( ... )                                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		mclo::detail::assert_failed( "UNREACHABLE", __FILE__, __LINE__, __VA_ARGS__ );                                 \
	}                                                                                                                  \
	while ( 0 )
#endif

#endif

// todo(mc) replace these with the direct MCLO prefixed one
#define DEBUG_ASSERT( EXPR, ... ) MCLO_DEBUG_ASSERT( EXPR, __VA_ARGS__ )
#define ASSERT( EXPR, ... ) MCLO_ASSERT( EXPR, __VA_ARGS__ )
#define ASSUME( EXPR, ... ) MCLO_ASSUME( EXPR, __VA_ARGS__ )
#define PANIC( ... ) MCLO_PANIC( __VA_ARGS__ )
#define UNREACHABLE( ... ) MCLO_UNREACHABLE( __VA_ARGS__ )
