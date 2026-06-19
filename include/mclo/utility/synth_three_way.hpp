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

	struct synth_three_way
	{
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

	template <typename T, typename U = T>
	using synth_three_way_result = decltype( synth_three_way{}( std::declval<T&>(), std::declval<U&>() ) );
}
