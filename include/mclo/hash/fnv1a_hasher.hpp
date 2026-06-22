#pragma once

#include "mclo/container/span.hpp"

#include <cstddef>

namespace mclo
{
	/// @brief A hasher implementing the 64-bit FNV-1a algorithm.
	/// @details A simple, dependency-free hasher satisfying the @ref hasher concept. Fast for small inputs but with
	/// weaker distribution than the other provided hashers; prefer @ref xxhash_64 or @ref rapidhash for general use.
	class fnv1a_hasher
	{
		static inline constexpr std::size_t offset_basis = 14695981039346656037ull;
		static inline constexpr std::size_t prime = 1099511628211ull;

	public:
		/// @brief Mixes @p data into the running hash.
		/// @param data The bytes to hash.
		void write( const mclo::span<const std::byte> data ) noexcept
		{
			for ( const std::byte byte : data )
			{
				m_hash ^= std::to_integer<std::uint8_t>( byte );
				m_hash *= prime;
			}
		}

		/// @brief Returns the final hash value.
		/// @return The accumulated hash.
		[[nodiscard]] std::size_t finish() const noexcept
		{
			return m_hash;
		}

	private:
		std::size_t m_hash = offset_basis;
	};
}
