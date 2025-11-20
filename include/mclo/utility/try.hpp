#pragma once

#include "expected.hpp"
#include "mclo/preprocessor/concat.hpp"

namespace mclo
{
	template <typename T>
	struct try_traits;

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

#define MCLO_TRY( DECLARATION, ... ) MCLO_TRY_INTERNAL( DECLARATION =, __VA_ARGS__ )
#define MCLO_TRY_VOID( ... ) MCLO_TRY_INTERNAL( ( void ), __VA_ARGS__ )
