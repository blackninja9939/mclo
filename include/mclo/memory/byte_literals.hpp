#pragma once

#include <cstddef>

namespace mclo
{
	inline namespace literals
	{
		inline namespace byte_literals
		{
			/// @brief User-defined literal yielding a count of kibibytes (1024 bytes) as a @c std::size_t.
			[[nodiscard]] constexpr std::size_t operator""_KiB( unsigned long long int x ) noexcept
			{
				return 1024ULL * x;
			}

			/// @brief User-defined literal yielding a count of mebibytes (1024 KiB) as a @c std::size_t.
			[[nodiscard]] constexpr std::size_t operator""_MiB( unsigned long long int x ) noexcept
			{
				return 1024_KiB * x;
			}

			/// @brief User-defined literal yielding a count of gibibytes (1024 MiB) as a @c std::size_t.
			[[nodiscard]] constexpr std::size_t operator""_GiB( unsigned long long int x ) noexcept
			{
				return 1024_MiB * x;
			}

			/// @brief User-defined literal yielding a count of tebibytes (1024 GiB) as a @c std::size_t.
			[[nodiscard]] constexpr std::size_t operator""_TiB( unsigned long long int x ) noexcept
			{
				return 1024_GiB * x;
			}

			/// @brief User-defined literal yielding a count of pebibytes (1024 TiB) as a @c std::size_t.
			[[nodiscard]] constexpr std::size_t operator""_PiB( unsigned long long int x ) noexcept
			{
				return 1024_TiB * x;
			}
		}
	}
}
