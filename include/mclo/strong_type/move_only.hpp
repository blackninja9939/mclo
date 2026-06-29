#pragma once

namespace mclo::strong_type
{
	/// @brief Mixin that makes a strong type move-only by deleting its copy operations.
	struct move_only
	{
		template <typename Derived>
		struct mixin
		{
			mixin() = default;
			mixin( const mixin& ) = delete;
			mixin& operator=( const mixin& ) = delete;
			mixin( mixin&& ) = default;
			mixin& operator=( mixin&& ) = default;
		};
	};
}
