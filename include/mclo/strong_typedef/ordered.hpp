#pragma once

#include "mclo/utility/synth_three_way.hpp"

namespace mclo
{
	/// @brief Mixin that adds homogeneous ordering via operator<=> along with equality.
	/// @details Provides the full set of relational operators and equality. The spaceship operator is synthesised via
	/// mclo::synth_three_way, so it also works for value types that only provide operator< (yielding
	/// std::weak_ordering) as well as those with a native operator<=>. Do not combine with equality_comparable as the
	/// duplicated equality operator would be ambiguous.
	struct ordered
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr auto operator<=>( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( synth_three_way{}( lhs.value, rhs.value ) ) )
			{
				return synth_three_way{}( lhs.value, rhs.value );
			}
			[[nodiscard]] friend constexpr bool operator==( const Derived& lhs,
															const Derived& rhs ) noexcept( noexcept( lhs.value ==
																									 rhs.value ) )
			{
				return lhs.value == rhs.value;
			}
		};
	};

	/// @brief Parameterised mixin that adds heterogeneous ordering and equality against another type.
	/// @details Adds operator<=> and operator== comparing the wrapped value directly against a T, with the reversed and
	/// relational operators synthesised by the compiler. The spaceship operator uses mclo::synth_three_way so it also
	/// supports value types offering only operator<. T is compared against the underlying value, not against a
	/// T::value, so to compare against another strong type target its underlying type or make that type implicitly
	/// convertible. Avoid pairing with implicitly_convertible against the same T to prevent ambiguity.
	/// @tparam T The type to compare the wrapped value against.
	template <typename T>
	struct ordered_with
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr auto operator<=>( const Derived& lhs, const T& rhs ) noexcept(
				noexcept( synth_three_way{}( lhs.value, rhs ) ) )
			{
				return synth_three_way{}( lhs.value, rhs );
			}
			[[nodiscard]] friend constexpr bool operator==( const Derived& lhs,
															const T& rhs ) noexcept( noexcept( lhs.value == rhs ) )
			{
				return lhs.value == rhs;
			}
		};
	};
}
