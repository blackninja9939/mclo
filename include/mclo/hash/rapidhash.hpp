#pragma once

#include "mclo/container/span.hpp"

#include <cstddef>
#include <cstdint>

namespace mclo
{
	/// @brief A hasher implementing the 64-bit rapidhash algorithm.
	/// @details Satisfies the @ref hasher concept and offers excellent speed and distribution, making it a strong
	/// general-purpose choice for the @ref default_hasher.
	class rapidhash
	{
	public:
		/// @brief Constructs the hasher with an optional seed.
		/// @param seed The seed value mixed into the hash.
		rapidhash( const std::uint64_t seed = 0xbdd89aa982704029 ) noexcept;

		/// @brief Mixes @p data into the running hash.
		/// @param data The bytes to hash.
		void write( const mclo::span<const std::byte> data ) noexcept;

		/// @brief Finalises and returns the hash value.
		/// @return The computed hash.
		[[nodiscard]] std::size_t finish() noexcept;

	private:
		std::uint64_t m_seed;
		std::uint64_t m_a = 0;
		std::uint64_t m_b = 0;
		std::size_t m_size = 0;
	};
}
