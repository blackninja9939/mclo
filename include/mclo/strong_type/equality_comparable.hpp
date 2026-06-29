#pragma once

namespace mclo::strong_type
{
	/// @brief Mixin that adds homogeneous equality comparison (== and !=).
	struct equality_comparable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr bool operator==( const Derived& lhs,
															const Derived& rhs ) noexcept( noexcept( lhs.value ==
																									 rhs.value ) )
			{
				return lhs.value == rhs.value;
			}
		};
	};

	/// @brief Parameterised mixin that adds heterogeneous equality comparison against another type.
	/// @details Adds operator== comparing the wrapped value directly against a T, with operator!= and the reversed
	/// forms synthesised by the compiler. T is compared against the underlying value, not against a T::value, so to
	/// compare against another strong type target its underlying type or make that type implicitly convertible. Avoid
	/// pairing with implicitly_convertible against the same T to prevent ambiguity.
	/// @tparam T The type to compare the wrapped value against.
	template <typename T>
	struct equality_comparable_with
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr bool operator==( const Derived& lhs,
															const T& rhs ) noexcept( noexcept( lhs.value == rhs ) )
			{
				return lhs.value == rhs;
			}
		};
	};
}
