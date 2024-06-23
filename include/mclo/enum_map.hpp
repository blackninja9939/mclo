#pragma once

#include <magic_enum.hpp>
#include <magic_enum_containers.hpp>
#include <optional>
#include <type_traits>

namespace mclo
{
	template <typename T>
	constexpr bool is_contiguous_enum()
	{
		constexpr auto values = magic_enum::enum_values<T>();
		constexpr auto count = values.size();
		for ( std::size_t index = 0; index < count; ++index )
		{
			if ( magic_enum::enum_integer( values[ index ] ) != index )
			{
				return false;
			}
		}
		return true;
	}

	struct enum_contiguous_indexing
	{
		using is_transparent = std::true_type;

		struct index_wrapper
		{
			constexpr explicit operator bool() const noexcept
			{
				return true;
			}
			constexpr std::size_t operator*() const noexcept
			{
				return value;
			}

			std::size_t value;
		};

		template <typename E>
		[[nodiscard]] static constexpr index_wrapper at( E val ) noexcept
		{
			static_assert( is_contiguous_enum<E>(),
						   "The values of the enum must match their indexes, that is the first enum constant has a "
						   "value of 0 and all following ones are contiguous values" );
			return index_wrapper{ static_cast<std::size_t>( magic_enum::enum_integer( val ) ) };
		}
	};

	template <typename TEnum, typename TValue>
	using enum_map = magic_enum::containers::array<TEnum, TValue, enum_contiguous_indexing>;
}
