#pragma once

#include <cstddef>

namespace mclo
{
	inline namespace literals
	{
		inline namespace byte_literals
		{
			[[nodsicard]] constexpr std::size_t operator""_KiB( unsigned long long int x ) noexcept
			{
				return 1024ULL * x;
			}

			[[nodsicard]] constexpr std::size_t operator""_MiB( unsigned long long int x ) noexcept
			{
				return 1024_KiB * x;
			}

			[[nodsicard]] constexpr std::size_t operator""_GiB( unsigned long long int x ) noexcept
			{
				return 1024_MiB * x;
			}

			[[nodsicard]] constexpr std::size_t operator""_TiB( unsigned long long int x ) noexcept
			{
				return 1024_GiB * x;
			}

			[[nodsicard]] constexpr std::size_t operator""_PiB( unsigned long long int x ) noexcept
			{
				return 1024_TiB * x;
			}
		}
	}
}
