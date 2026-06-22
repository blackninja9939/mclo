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

	/// @brief The number of enumerators in @p TEnum, defining the bounds used by the enum containers.
	/// @details A customization point for the enum container types (@c enum_map, @c enum_set, etc.): an enumeration
	/// opts in by declaring a member named @c enum_size, conventionally a trailing sentinel enumerator equal to the
	/// count of real values. When no such member exists this resolves to an internal undefined marker and
	/// @ref has_enum_size is @c false.
	/// @tparam TEnum The enumeration type to query.
	template <typename TEnum>
	constexpr auto enum_size = detail::member_enum_size<TEnum>();

	/// @brief Concept satisfied when @p TEnum provides a size through @ref enum_size.
	/// @tparam TEnum The enumeration type to test.
	template <typename TEnum>
	concept has_enum_size = !std::same_as<std::decay_t<decltype( enum_size<TEnum> )>, detail::undefined_enum_size>;
}
