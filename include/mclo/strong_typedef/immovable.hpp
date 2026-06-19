#pragma once

namespace mclo
{
	/// @brief Mixin that makes a strong_typedef neither copyable nor movable.
	struct immovable
	{
		template <typename Derived>
		struct mixin
		{
			mixin() = default;
			mixin( const mixin& ) = delete;
			mixin& operator=( const mixin& ) = delete;
			mixin( mixin&& ) = delete;
			mixin& operator=( mixin&& ) = delete;
		};
	};
}
