#pragma once

#include "mclo/platform/attributes.hpp"

#include <iosfwd>

namespace mclo::strong_type
{
	/// @brief Mixin that opts a strong type into stream extraction via operator>>.
	struct istreamable
	{
		template <typename Derived>
		struct mixin
		{
			template <typename CharT, typename Traits>
			friend std::basic_istream<CharT, Traits>& operator>>( std::basic_istream<CharT, Traits>& is,
																  Derived& object )
			{
				return is >> object.value;
			}
		};
	};

	/// @brief Mixin that opts a strong type into stream insertion via operator<<.
	struct ostreamable
	{
		template <typename Derived>
		struct mixin
		{
			template <typename CharT, typename Traits>
			friend std::basic_ostream<CharT, Traits>& operator<<( std::basic_ostream<CharT, Traits>& os,
																  const Derived& object )
			{
				return os << object.value;
			}
		};
	};

	/// @brief Preset bundle that opts a strong type into both stream insertion and extraction.
	struct iostreamable
	{
		template <typename Derived>
		struct MCLO_EMPTY_BASES mixin : istreamable::mixin<Derived>, ostreamable::mixin<Derived>
		{
		};
	};
}
