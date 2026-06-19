#pragma once

#include "mclo/strong_typedef/decrementable.hpp"
#include "mclo/strong_typedef/incrementable.hpp"
#include "mclo/strong_typedef/strong_typedef.hpp"

#include "mclo/platform/attributes.hpp"

#include <limits>

namespace mclo
{
	/// @brief Mixin that adds homogeneous addition (operator+ and operator+=).
	struct addable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator+( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value + rhs.value ) ) )
			{
				return Derived( lhs.value + rhs.value );
			}
			friend constexpr Derived& operator+=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value += rhs.value ) )
			{
				lhs.value += rhs.value;
				return lhs;
			}
		};
	};

	/// @brief Mixin that adds homogeneous subtraction (operator- and operator-=).
	struct subtractable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator-( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value - rhs.value ) ) )
			{
				return Derived( lhs.value - rhs.value );
			}
			friend constexpr Derived& operator-=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value -= rhs.value ) )
			{
				lhs.value -= rhs.value;
				return lhs;
			}
		};
	};

	/// @brief Mixin that adds homogeneous multiplication (operator* and operator*=).
	struct multipliable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator*( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value * rhs.value ) ) )
			{
				return Derived( lhs.value * rhs.value );
			}
			friend constexpr Derived& operator*=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value *= rhs.value ) )
			{
				lhs.value *= rhs.value;
				return lhs;
			}
		};
	};

	/// @brief Mixin that adds homogeneous division (operator/ and operator/=).
	struct dividable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator/( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value / rhs.value ) ) )
			{
				return Derived( lhs.value / rhs.value );
			}
			friend constexpr Derived& operator/=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value /= rhs.value ) )
			{
				lhs.value /= rhs.value;
				return lhs;
			}
		};
	};

	/// @brief Mixin that adds homogeneous modulo (operator% and operator%=).
	struct modulable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator%( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value % rhs.value ) ) )
			{
				return Derived( lhs.value % rhs.value );
			}
			friend constexpr Derived& operator%=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value %= rhs.value ) )
			{
				lhs.value %= rhs.value;
				return lhs;
			}
		};
	};

	/// @brief Mixin that adds unary negation (unary operator-).
	struct negatable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator-( const Derived& self ) noexcept(
				noexcept( Derived( -self.value ) ) )
			{
				return Derived( -self.value );
			}
		};
	};

	/// @brief Preset bundle composing the full set of arithmetic mixins.
	struct arithmetic
	{
		template <typename Derived>
		struct MCLO_EMPTY_BASES mixin : addable::mixin<Derived>,
										subtractable::mixin<Derived>,
										multipliable::mixin<Derived>,
										dividable::mixin<Derived>,
										modulable::mixin<Derived>,
										negatable::mixin<Derived>,
										incrementable::mixin<Derived>,
										decrementable::mixin<Derived>
		{
		};
	};
}

namespace std
{
	template <typename Wrapped, typename Tag, typename... Mixins>
		requires mclo::has_mixin<mclo::strong_typedef<Wrapped, Tag, Mixins...>, mclo::arithmetic>
	struct numeric_limits<mclo::strong_typedef<Wrapped, Tag, Mixins...>> : numeric_limits<Wrapped>
	{
		using strong_type = mclo::strong_typedef<Wrapped, Tag, Mixins...>;

		[[nodiscard]] static constexpr strong_type min() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::min() );
		}
		[[nodiscard]] static constexpr strong_type lowest() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::lowest() );
		}
		[[nodiscard]] static constexpr strong_type max() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::max() );
		}
		[[nodiscard]] static constexpr strong_type epsilon() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::epsilon() );
		}
		[[nodiscard]] static constexpr strong_type round_error() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::round_error() );
		}
		[[nodiscard]] static constexpr strong_type infinity() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::infinity() );
		}
		[[nodiscard]] static constexpr strong_type quiet_NaN() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::quiet_NaN() );
		}
		[[nodiscard]] static constexpr strong_type signaling_NaN() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::signaling_NaN() );
		}
		[[nodiscard]] static constexpr strong_type denorm_min() noexcept
		{
			return strong_type( numeric_limits<Wrapped>::denorm_min() );
		}
	};
}
