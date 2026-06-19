#pragma once

namespace mclo
{
	/// @brief Parameterised mixin that adds subtraction of two objects yielding a distinct Difference type.
	/// @details Models pointer or iterator style differences where subtracting two strong values produces a separate
	/// type rather than another value of the same strong type. Contrast with subtractable, whose operator- returns the
	/// same strong type.
	/// @tparam Difference The result type of subtracting two Derived values.
	template <typename Difference>
	struct difference
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Difference operator-( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Difference( lhs.value - rhs.value ) ) )
			{
				return Difference( lhs.value - rhs.value );
			}
		};
	};
}
