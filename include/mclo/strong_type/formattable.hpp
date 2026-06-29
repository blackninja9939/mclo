#pragma once

namespace mclo::strong_type
{
	/// @brief Mixin that opts a strong type into formatting by exposing its value via format_as.
	struct formattable
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] friend constexpr const auto& format_as( const Derived& object ) noexcept
			{
				return object.value;
			}
		};
	};
}
