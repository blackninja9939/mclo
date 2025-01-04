#pragma once

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
		[[nodiscard]] constexpr auto operator()( const T& lhs, const U& rhs )
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
