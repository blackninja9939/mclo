#pragma once

#include <concepts>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		struct undefined_enum_size
		{
		};

		template <typename TEnum>
		[[nodiscard]] constexpr auto member_enum_size()
		{
			if constexpr ( requires { TEnum::enum_size; } )
			{
				return TEnum::enum_size;
			}
			else
			{
				return undefined_enum_size{};
			}
		}
	}

	template <typename TEnum>
	constexpr auto enum_size = detail::member_enum_size<TEnum>();

	template <typename TEnum>
	concept has_enum_size = !std::same_as<std::decay_t<decltype( enum_size<TEnum> )>, detail::undefined_enum_size>;
}
