#pragma once

#include "mclo/always_false.hpp"

#include <type_traits>

namespace mclo
{
	namespace detail
	{
		template <typename TEnum>
		[[nodiscard]] constexpr TEnum member_enum_size()
		{
			if constexpr ( requires { TEnum::Size; } )
			{
				return TEnum::Size;
			}
			else if constexpr ( requires { TEnum::size; } )
			{
				return TEnum::size;
			}
			else if constexpr ( requires { TEnum::_Size; } )
			{
				return TEnum::_Size;
			}
			else if constexpr ( requires { TEnum::_size; } )
			{
				return TEnum::_size;
			}
			else if constexpr ( requires { TEnum::Count; } )
			{
				return TEnum::Count;
			}
			else if constexpr ( requires { TEnum::count; } )
			{
				return TEnum::count;
			}
			else if constexpr ( requires { TEnum::_Count; } )
			{
				return TEnum::_Count;
			}
			else if constexpr ( requires { TEnum::_count; } )
			{
				return TEnum::_count;
			}
			else
			{
				static_assert( mclo::always_false<TEnum>,
							   "No enum matching size pattern, add one, specialize the enum_size variable, or specify "
							   "the size enum manually at the call site" );
			}
		}

		template <typename T>
		[[nodiscard]] constexpr auto to_underlying( const T value ) noexcept
		{
			return static_cast<std::underlying_type_t<T>>( value );
		}
	}

	template <typename TEnum>
	constexpr TEnum enum_size = detail::member_enum_size<TEnum>();
}
