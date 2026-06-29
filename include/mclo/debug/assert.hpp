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
#include "mclo/platform/pretty_function.hpp"

#include <format>

namespace mclo::detail
{
	[[noreturn]] void assert_failed( const char* expression,
									 const char* function,
									 const char* file,
									 const unsigned line,
									 const char* message,
									 const char* formatted_args ) noexcept;

	// We breakpoint before calling the core assert_failed in case the assertion failure itself throws bad_alloc when
	// formatting the message. This allows the user to inspect the assertion in a debugger before the program aborts.

	[[noreturn]] inline void assert_failed( const char* expression,
											const char* function,
											const char* file,
											unsigned line ) noexcept
	{
		mclo::breakpoint_if_debugging();
		assert_failed( expression, function, file, line, nullptr, nullptr );
	}

	[[noreturn]] inline void assert_failed(
		const char* expression, const char* function, const char* file, unsigned line, const char* message ) noexcept
	{
		mclo::breakpoint_if_debugging();
		assert_failed( expression, function, file, line, message, nullptr );
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
	[[noreturn]] void assert_failed( const char* expression,
									 const char* function,
									 const char* file,
									 unsigned line,
									 const char* message,
									 const Args&... args ) noexcept
	{
		mclo::breakpoint_if_debugging();

		std::string arg_string;
		arg_string.reserve( sizeof...( Args ) * 24 );

		std::size_t index = 0;
		( format_assert_arg( arg_string, index++, args ), ... );

		assert_failed( expression, function, file, line, message, arg_string.c_str() );
	}
}

#ifdef NDEBUG
#define MCLO_DETAIL_DEBUG_ASSERT( EXPR, ... ) ( ( void )0 )
#else
#define MCLO_DETAIL_DEBUG_ASSERT( EXPR, ... )                                                                          \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( EXPR ) ) [[unlikely]]                                                                                  \
		{                                                                                                              \
			mclo::detail::assert_failed( #EXPR, MCLO_PRETTY_FUNCTION, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__ ) ); \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )
#endif

#define MCLO_DETAIL_ASSERT( EXPR, ... )                                                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( EXPR ) ) [[unlikely]]                                                                                  \
		{                                                                                                              \
			mclo::detail::assert_failed( #EXPR, MCLO_PRETTY_FUNCTION, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__ ) ); \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )

#ifdef NDEBUG
// Prefer the cross platform attribute when available, otherwise fall back to platform specific extensions
#if __has_cpp_attribute( assume )
#define MCLO_DETAIL_ASSUME( EXPR, ... ) [[assume( EXPR )]]
#elif defined( MCLO_COMPILER_CLANG )
#define MCLO_DETAIL_ASSUME( EXPR, ... ) __builtin_assume( EXPR )
#elif defined( MCLO_COMPILER_GCC )
#define MCLO_DETAIL_ASSUME( EXPR, ... ) __attribute__( ( assume( EXPR ) ) )
#elif defined( MCLO_COMPILER_MSVC )
#define MCLO_DETAIL_ASSUME( EXPR, ... ) __assume( EXPR )
#else
#define MCLO_DETAIL_ASSUME( EXPR, ... )
#endif
#else
#define MCLO_DETAIL_ASSUME( EXPR, ... )                                                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		if ( !( EXPR ) ) [[unlikely]]                                                                                  \
		{                                                                                                              \
			mclo::detail::assert_failed( #EXPR, MCLO_PRETTY_FUNCTION, __FILE__, __LINE__ __VA_OPT__(, __VA_ARGS__ ) ); \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )
#endif

#define MCLO_DETAIL_PANIC( ... )                                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		mclo::detail::assert_failed( "PANIC", MCLO_PRETTY_FUNCTION, __FILE__, __LINE__, __VA_ARGS__ );                 \
	}                                                                                                                  \
	while ( 0 )

#ifdef NDEBUG
#define MCLO_DETAIL_UNREACHABLE( ... ) mclo::unreachable()
#else
#define MCLO_DETAIL_UNREACHABLE( ... )                                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		mclo::detail::assert_failed( "UNREACHABLE", MCLO_PRETTY_FUNCTION, __FILE__, __LINE__, __VA_ARGS__ );           \
	}                                                                                                                  \
	while ( 0 )
#endif

/// @brief Checks a programmer-error precondition in debug builds; compiled out entirely under @c NDEBUG.
/// @details Use for internal invariants that should be caught and fixed before shipping. The condition is not even
/// evaluated in release, so it must be free of required side effects.
/// @param EXPR The condition that must hold.
/// @param ... Optional message string followed by optional variables whose values are printed.
#ifndef MCLO_DEBUG_ASSERT
#define MCLO_DEBUG_ASSERT( EXPR, ... ) MCLO_DETAIL_DEBUG_ASSERT( EXPR __VA_OPT__(, __VA_ARGS__ ) )
#endif

/// @brief Checks a non-recoverable invariant in both debug and release builds.
/// @details Use for invariants that cannot be recovered from but could occur due to invalid user input. Unlike
/// @ref MCLO_DEBUG_ASSERT the condition is always evaluated and checked.
/// @param EXPR The condition that must hold.
/// @param ... Optional message string followed by optional variables whose values are printed.
#ifndef MCLO_ASSERT
#define MCLO_ASSERT( EXPR, ... ) MCLO_DETAIL_ASSERT( EXPR __VA_OPT__(, __VA_ARGS__ ) )
#endif

/// @brief Checks a condition in debug builds; in release acts as an optimisation hint that the condition is true.
/// @details Use for conditions that are impossible by design. In release builds a false condition is undefined
/// behaviour rather than a checked failure, so the condition must be free of required side effects.
/// @param EXPR The condition that is assumed to hold.
/// @param ... Optional message string followed by optional variables whose values are printed.
#ifndef MCLO_ASSUME
#define MCLO_ASSUME( EXPR, ... ) MCLO_DETAIL_ASSUME( EXPR __VA_OPT__(, __VA_ARGS__ ) )
#endif

/// @brief Unconditionally fails in both debug and release builds; an unconditional @ref MCLO_ASSERT.
/// @details Use for code paths that should never run. A message is required.
/// @param ... The message string followed by optional variables whose values are printed.
#ifndef MCLO_PANIC
#define MCLO_PANIC( ... ) MCLO_DETAIL_PANIC( __VA_ARGS__ )
#endif

/// @brief Fails in debug builds; in release acts as an optimisation hint that the path is unreachable.
/// @details Use for code paths that should be unreachable; an unconditional @ref MCLO_ASSUME. A message is required.
/// @param ... The message string followed by optional variables whose values are printed.
#ifndef MCLO_UNREACHABLE
#define MCLO_UNREACHABLE( ... ) MCLO_DETAIL_UNREACHABLE( __VA_ARGS__ )
#endif
