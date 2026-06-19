#pragma once

namespace mclo
{
	/// @brief Parameterised mixin adding multiplication and division by a separate scalar type Scalar.
	/// @details Models scaling a strong value by a plain scalar, e.g. mclo::strong_typedef<double, struct tag,
	/// mclo::scalable_with<double>> so that meters * 2.0 yields meters, without permitting the dimensionally
	/// meaningless strong * strong product that multipliable would allow. Multiplication is provided with the scalar on
	/// either side; division is only provided as Derived / Scalar. Compound assignment forms (operator*= and
	/// operator/=) are also generated.
	/// @tparam Scalar The scalar type the strong value can be scaled by.
	template <typename Scalar>
	struct scalable_with
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator*( const Derived& lhs, const Scalar& rhs ) noexcept(
				noexcept( Derived( lhs.value * rhs ) ) )
			{
				return Derived( lhs.value * rhs );
			}
			[[nodiscard]] friend constexpr Derived operator*( const Scalar& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs * rhs.value ) ) )
			{
				return Derived( lhs * rhs.value );
			}
			[[nodiscard]] friend constexpr Derived operator/( const Derived& lhs, const Scalar& rhs ) noexcept(
				noexcept( Derived( lhs.value / rhs ) ) )
			{
				return Derived( lhs.value / rhs );
			}
			friend constexpr Derived& operator*=( Derived& lhs,
												  const Scalar& rhs ) noexcept( noexcept( lhs.value *= rhs ) )
			{
				lhs.value *= rhs;
				return lhs;
			}
			friend constexpr Derived& operator/=( Derived& lhs,
												  const Scalar& rhs ) noexcept( noexcept( lhs.value /= rhs ) )
			{
				lhs.value /= rhs;
				return lhs;
			}
		};
	};
}
