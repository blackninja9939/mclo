#pragma once

#include <cstddef>
#include <span>

namespace mclo
{
	class fnv1a_hasher
	{
		static inline constexpr std::size_t offset_basis = 14695981039346656037ull;
		static inline constexpr std::size_t prime = 14695981039346656037ull;

	public:
		void write( const std::span<const std::byte> data ) noexcept
		{
			for ( const std::byte byte : data )
			{
				m_hash ^= std::to_integer<std::uint8_t>( byte );
				m_hash *= prime;
			}
		}

		[[nodiscard]] std::size_t finish() const noexcept
		{
			return m_hash;
		}

	private:
		std::size_t m_hash = offset_basis;
	};
}
