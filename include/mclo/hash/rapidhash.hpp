#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace mclo
{
	class rapidhash
	{
	public:
		rapidhash( const std::uint64_t seed = 0xbdd89aa982704029 ) noexcept;

		void write( const std::span<const std::byte> data ) noexcept;
		[[nodiscard]] std::size_t finish() noexcept;

	private:
		std::uint64_t m_seed;
		std::uint64_t m_a = 0;
		std::uint64_t m_b = 0;
		std::size_t m_size = 0;
	};
}
