#pragma once

#include "mclo/platform/cpp_feature_compat.hpp"

#include <compare>
#include <concepts>

namespace mclo
{
	namespace detail
	{
		template <class B>
		concept ConvertsToBool = std::convertible_to<B, bool>;

		template <class B>
		concept BooleanTestable = ConvertsToBool<B> && requires( B&& b ) {
			{ !std::forward<B>( b ) } -> ConvertsToBool;
		};
	}

	/// @brief Function object that yields a three way comparison result even for types lacking @c operator<=>.
	/// @details Mirrors the standard library's exposition only @c synth-three-way helper. If the operands are
	/// three way comparable it forwards to @c operator<=>; otherwise it synthesises a @c std::weak_ordering from
	/// two @c operator< comparisons. Useful when defaulting @c operator<=> for a type containing members that only
	/// provide @c operator<.
	struct synth_three_way
	{
		/// @brief Compares two values, returning a three way ordering.
		/// @tparam T The type of the left hand operand.
		/// @tparam U The type of the right hand operand.
		/// @param lhs The left hand operand.
		/// @param rhs The right hand operand.
		/// @return The result of @c lhs <=> rhs if available, otherwise a @c std::weak_ordering synthesised from
		/// @c operator<.
		template <typename T, typename U>
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR constexpr auto operator()( const T& lhs,
																		   const U& rhs ) MCLO_CONST_CALL_OPERATOR
			noexcept( noexcept( lhs < rhs ) && noexcept( rhs < lhs ) )
			requires requires {
				{ lhs < rhs } -> detail::BooleanTestable;
				{ rhs < lhs } -> detail::BooleanTestable;
			}
		{
			if constexpr ( std::three_way_comparable_with<T, U> )
			{
				return lhs <=> rhs;
			}
			else
			{
				if ( lhs < rhs )
				{
					return std::weak_ordering::less;
				}
				if ( rhs < lhs )
				{
					return std::weak_ordering::greater;
				}
				return std::weak_ordering::equivalent;
			}
		}
	};

	/// @brief The ordering category produced by @ref synth_three_way for the given operand types.
	/// @tparam T The type of the left hand operand.
	/// @tparam U The type of the right hand operand, defaulting to @p T.
	template <typename T, typename U = T>
	using synth_three_way_result = decltype( synth_three_way{}( std::declval<T&>(), std::declval<U&>() ) );
}
