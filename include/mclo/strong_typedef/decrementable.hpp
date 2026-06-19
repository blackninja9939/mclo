#pragma once

namespace mclo
{
	/// @brief Mixin that adds pre and post decrement (operator--).
	struct decrementable
	{
		template <typename Derived>
		struct mixin
		{
			friend constexpr Derived& operator--( Derived& self ) noexcept( noexcept( --self.value ) )
			{
				--self.value;
				return self;
			}
			friend constexpr Derived operator--( Derived& self, int ) noexcept( noexcept( Derived( self ) ) &&
																				noexcept( --self.value ) )
			{
				Derived old( self );
				--self.value;
				return old;
			}
		};
	};
}
