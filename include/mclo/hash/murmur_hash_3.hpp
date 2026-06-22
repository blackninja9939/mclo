#pragma once

#include "mclo/container/span.hpp"

#include <cstddef>
#include <cstdint>

namespace mclo
{
	/// @brief A hasher implementing the 32-bit MurmurHash3 algorithm.
	/// @details Satisfies the @ref hasher concept, accumulating input incrementally across @c write calls and
	/// finalising with @c finish.
	class murmur_hash_3
	{
	public:
		/// @brief Constructs the hasher with an optional seed.
		/// @param seed The seed value mixed into the hash.
		explicit murmur_hash_3( const std::uint32_t seed = 0 ) noexcept
			: m_hash( seed )
		{
		}

		/// @brief Mixes @p data into the running hash.
		/// @param data The bytes to hash.
		void write( const mclo::span<const std::byte> data ) noexcept;

		/// @brief Finalises and returns the hash value.
		/// @return The computed hash.
		[[nodiscard]] std::size_t finish() noexcept;

	private:
		std::uint32_t m_hash = 0;
		std::uint32_t m_carry = 0;
		std::uint32_t m_total_length = 0;
	};
}
