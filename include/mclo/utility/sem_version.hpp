#pragma once

#include <cinttypes>
#include <compare>

#include "mclo/hash/hash_append.hpp"

namespace mclo
{
	struct sem_version final
	{
		std::uint8_t major;
		std::uint8_t minor;
		std::uint8_t patch;

		[[nodiscard]] constexpr bool satisfies( const sem_version& other ) const noexcept
		{
			if ( major != other.major )
			{
				// Major version must be the same, it represents a breaking change
				return false;
			}
			if ( minor < other.minor )
			{
				// Minor version must be greater or equal, it represents a backward compatible change
				return false;
			}

			// Patch and other flags do not take part in compatibility
			return true;
		}

		[[nodiscard]] constexpr std::strong_ordering operator<=>( const sem_version& other ) const noexcept = default;
	};
}
