#pragma once

#include "expected.hpp"
#include "mclo/preprocessor/concat.hpp"

namespace mclo
{
	/// @brief Customization point describing how to inspect and unwrap a "try-able" value for the @c MCLO_TRY macros.
	/// @details Specialise this template for a type to make it usable with @c MCLO_TRY. A specialisation must provide
	/// @c has_value, @c value and @c error static functions. The library ships specialisations for
	/// @c std::optional and @ref mclo::expected.
	/// @tparam T The value type being unwrapped.
	template <typename T>
	struct try_traits;

	/// @brief @ref try_traits specialisation for @c std::optional, returning @c std::nullopt on failure.
	template <typename T>
	struct try_traits<std::optional<T>>
	{
		static constexpr bool has_value( const std::optional<T>& value ) noexcept
		{
			return value.has_value();
		};

		template <typename U>
		static constexpr auto&& value( U&& value ) noexcept
		{
			return *std::forward<U>( value );
		};

		template <typename U>
		static constexpr std::nullopt_t error( U&& ) noexcept
		{
			return std::nullopt;
		};
	};

	/// @brief @ref try_traits specialisation for @ref mclo::expected, returning an @c unexpected on failure.
	template <typename T, typename E>
	struct try_traits<expected<T, E>>
	{
		static constexpr bool has_value( const expected<T, E>& value ) noexcept
		{
			return value.has_value();
		};

		template <typename U>
		static constexpr auto&& value( U&& value ) noexcept
		{
			return *std::forward<U>( value );
		};

		template <typename U>
		static constexpr auto error( U&& value ) noexcept
		{
			return mclo::unexpected<E>{ std::forward<U>( value ).error() };
		};
	};

	/// @brief @ref try_traits specialisation for @c expected<void, E>, which carries no success value.
	template <typename E>
	struct try_traits<expected<void, E>>
	{
		static constexpr bool has_value( const expected<void, E>& value ) noexcept
		{
			return value.has_value();
		};

		template <typename U>
		static constexpr void value( U&& ) noexcept {};

		template <typename U>
		static constexpr auto error( U&& value ) noexcept
		{
			return mclo::unexpected<E>{ std::forward<U>( value ).error() };
		};
	};
}

#define MCLO_TRY_VARIABLE_NAME MCLO_CONCAT( mclo_try_variable_, __LINE__ )
#define MCLO_TRY_VARIABLE_TYPE decltype( MCLO_TRY_VARIABLE_NAME )
#define MCLO_TRY_VARIABLE_TRAITS mclo::try_traits<std::remove_cvref_t<MCLO_TRY_VARIABLE_TYPE>>

#define MCLO_TRY_INTERNAL( DECLARATION, ... )                                                                          \
	auto&& MCLO_TRY_VARIABLE_NAME = __VA_ARGS__;                                                                       \
	if ( !MCLO_TRY_VARIABLE_TRAITS::has_value( MCLO_TRY_VARIABLE_NAME ) )                                              \
	{                                                                                                                  \
		return MCLO_TRY_VARIABLE_TRAITS::error( std::forward<MCLO_TRY_VARIABLE_TYPE>( MCLO_TRY_VARIABLE_NAME ) );      \
	}                                                                                                                  \
	DECLARATION MCLO_TRY_VARIABLE_TRAITS::value( std::forward<MCLO_TRY_VARIABLE_TYPE>( MCLO_TRY_VARIABLE_NAME ) )

/// @brief Evaluates an expression and, on failure, returns its error from the enclosing function; on success binds
/// the unwrapped value.
/// @details The expression must produce a type supported by @ref mclo::try_traits, such as @c std::optional or
/// @ref mclo::expected. If it holds a failure the enclosing function returns the corresponding error (for example
/// @c std::nullopt or an @c unexpected); otherwise the success value is bound using @p DECLARATION.
/// @param DECLARATION The variable declaration that receives the unwrapped success value, for example @c auto x.
/// @param ... The expression to evaluate and unwrap.
/// @code
/// mclo::expected<int, std::error_code> compute()
/// {
///     MCLO_TRY( const auto value, may_fail() );
///     return value * 2;
/// }
/// @endcode
#define MCLO_TRY( DECLARATION, ... ) MCLO_TRY_INTERNAL( DECLARATION =, __VA_ARGS__ )

/// @brief Like @ref MCLO_TRY but discards the success value, only propagating failure.
/// @details Use when the operation is performed for its side effects or carries no success value, such as
/// @c expected<void, E>.
/// @param ... The expression to evaluate and check for failure.
#define MCLO_TRY_VOID( ... ) MCLO_TRY_INTERNAL( ( void ), __VA_ARGS__ )
