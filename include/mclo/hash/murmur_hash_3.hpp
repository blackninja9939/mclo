#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace mclo
{
	class murmur_hash_3
	{
	public:
		explicit murmur_hash_3( const std::uint32_t seed = 0 ) noexcept
			: m_hash( seed )
		{
		}

		void write( const std::span<const std::byte> data ) noexcept;
		[[nodiscard]] std::size_t finish() noexcept;

	private:
		std::uint32_t m_hash = 0;
		std::uint32_t m_carry = 0;
		std::uint32_t m_total_length = 0;
	};
}
