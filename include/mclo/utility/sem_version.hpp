#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>

namespace mclo
{
	/// @brief A semantic version made up of major, minor and patch components.
	/// @details Follows semantic versioning conventions: a change in @ref major signals a breaking change, an
	/// increase in @ref minor a backward compatible addition, and @ref patch a backward compatible fix. Each
	/// component is stored in a single byte.
	struct sem_version final
	{
		/// @brief The major version, incremented for breaking changes.
		std::uint8_t major;

		/// @brief The minor version, incremented for backward compatible additions.
		std::uint8_t minor;

		/// @brief The patch version, incremented for backward compatible fixes.
		std::uint8_t patch;

		/// @brief Checks whether this version satisfies a required version.
		/// @details This version satisfies @p other when it has the same @ref major version and a @ref minor version
		/// greater than or equal to that of @p other. The @ref patch component does not affect compatibility.
		/// @param other The minimum required version.
		/// @return @c true if this version is compatible with and at least as new as @p other.
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

		/// @brief Defaulted three way comparison ordering by major, then minor, then patch.
		[[nodiscard]] constexpr std::strong_ordering operator<=>( const sem_version& other ) const noexcept = default;

		/// @brief Converts the version to a "major.minor.patch" string.
		/// @return The string representation of the version.
		std::string to_string() const;
	};

	/// @brief Formats a version as its string representation for use with fmtlib/std::format.
	/// @param version The version to format.
	/// @return The string representation of @p version.
	std::string format_as( const sem_version& version );
}
