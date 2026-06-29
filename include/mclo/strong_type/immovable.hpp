#pragma once

namespace mclo::strong_type
{
	/// @brief Mixin that makes a strong type neither copyable nor movable.
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
