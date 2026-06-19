#pragma once

#include <concepts>

namespace mclo
{
	/// @brief Mixin that adds homogeneous bitwise operators (&, |, ^, ~), shifts by an integral amount (<<, >>) and
	/// their compound assignments.
	struct bitwise
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr Derived operator&( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value & rhs.value ) ) )
			{
				return Derived( lhs.value & rhs.value );
			}
			[[nodiscard]] friend constexpr Derived operator|( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value | rhs.value ) ) )
			{
				return Derived( lhs.value | rhs.value );
			}
			[[nodiscard]] friend constexpr Derived operator^( const Derived& lhs, const Derived& rhs ) noexcept(
				noexcept( Derived( lhs.value ^ rhs.value ) ) )
			{
				return Derived( lhs.value ^ rhs.value );
			}
			[[nodiscard]] friend constexpr Derived operator~( const Derived& self ) noexcept(
				noexcept( Derived( ~self.value ) ) )
			{
				return Derived( ~self.value );
			}
			friend constexpr Derived& operator&=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value &= rhs.value ) )
			{
				lhs.value &= rhs.value;
				return lhs;
			}
			friend constexpr Derived& operator|=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value |= rhs.value ) )
			{
				lhs.value |= rhs.value;
				return lhs;
			}
			friend constexpr Derived& operator^=( Derived& lhs,
												  const Derived& rhs ) noexcept( noexcept( lhs.value ^= rhs.value ) )
			{
				lhs.value ^= rhs.value;
				return lhs;
			}
			template <std::integral Int>
			[[nodiscard]] friend constexpr Derived operator<<( const Derived& self, const Int shift ) noexcept(
				noexcept( Derived( self.value << shift ) ) )
			{
				return Derived( self.value << shift );
			}
			template <std::integral Int>
			[[nodiscard]] friend constexpr Derived operator>>( const Derived& self, const Int shift ) noexcept(
				noexcept( Derived( self.value >> shift ) ) )
			{
				return Derived( self.value >> shift );
			}
			template <std::integral Int>
			friend constexpr Derived& operator<<=( Derived& self,
												   const Int shift ) noexcept( noexcept( self.value <<= shift ) )
			{
				self.value <<= shift;
				return self;
			}
			template <std::integral Int>
			friend constexpr Derived& operator>>=( Derived& self,
												   const Int shift ) noexcept( noexcept( self.value >>= shift ) )
			{
				self.value >>= shift;
				return self;
			}
		};
	};
}
