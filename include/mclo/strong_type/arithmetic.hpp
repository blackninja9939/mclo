#pragma once

#include "mclo/strong_type/decrementable.hpp"
#include "mclo/strong_type/incrementable.hpp"
#include "mclo/strong_type/type.hpp"

#include "mclo/platform/attributes.hpp"

#include <limits>

namespace mclo::strong_type
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
		requires mclo::strong_type::has_mixin<mclo::strong_type::type<Wrapped, Tag, Mixins...>,
											  mclo::strong_type::arithmetic>
	struct numeric_limits<mclo::strong_type::type<Wrapped, Tag, Mixins...>> : numeric_limits<Wrapped>
	{
		using st = mclo::strong_type::type<Wrapped, Tag, Mixins...>;

		[[nodiscard]] static constexpr st min() noexcept
		{
			return st( numeric_limits<Wrapped>::min() );
		}
		[[nodiscard]] static constexpr st lowest() noexcept
		{
			return st( numeric_limits<Wrapped>::lowest() );
		}
		[[nodiscard]] static constexpr st max() noexcept
		{
			return st( numeric_limits<Wrapped>::max() );
		}
		[[nodiscard]] static constexpr st epsilon() noexcept
		{
			return st( numeric_limits<Wrapped>::epsilon() );
		}
		[[nodiscard]] static constexpr st round_error() noexcept
		{
			return st( numeric_limits<Wrapped>::round_error() );
		}
		[[nodiscard]] static constexpr st infinity() noexcept
		{
			return st( numeric_limits<Wrapped>::infinity() );
		}
		[[nodiscard]] static constexpr st quiet_NaN() noexcept
		{
			return st( numeric_limits<Wrapped>::quiet_NaN() );
		}
		[[nodiscard]] static constexpr st signaling_NaN() noexcept
		{
			return st( numeric_limits<Wrapped>::signaling_NaN() );
		}
		[[nodiscard]] static constexpr st denorm_min() noexcept
		{
			return st( numeric_limits<Wrapped>::denorm_min() );
		}
	};
}
