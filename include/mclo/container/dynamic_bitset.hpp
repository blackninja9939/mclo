
#pragma once

#include "mclo/container/detail/bitset_base.hpp"
#include "mclo/hash/hash.hpp"

#include <vector>

namespace mclo
{
	/// @brief Bitset with a dynamic size set at run time
	/// @tparam Allocator Allocator for underlying std::vector
	/// @tparam UnderlyingType Type of integer to store in the vector for the bitset
	template <std::unsigned_integral UnderlyingType = std::size_t, typename Allocator = std::allocator<UnderlyingType>>
	class dynamic_bitset
		: public detail::bitset_base<dynamic_bitset<UnderlyingType, Allocator>, std::vector<UnderlyingType, Allocator>>
	{
		using base =
			detail::bitset_base<dynamic_bitset<UnderlyingType, Allocator>, std::vector<UnderlyingType, Allocator>>;
		friend base;

	public:
		using underlying_container = typename base::underlying_container;
		using underlying_type = typename base::underlying_type;

		using base::base;

		/// @brief Construct the bitset with capacity for size bits
		/// @param size Number of bits to allocate space for
		explicit dynamic_bitset( const std::size_t size )
		{
			resize( size );
		}

		/// @brief Construct from a string like type of unset_char and set_char
		template <typename StringLike, typename CharT = typename StringLike::value_type>
		constexpr explicit dynamic_bitset( const StringLike& str,
										   const CharT unset_char = CharT( '0' ),
										   const CharT set_char = CharT( '1' ) )
			: dynamic_bitset( str.size() )
		{
			using view = std::basic_string_view<CharT, typename StringLike::traits_type>;
			base::init_from_string( view( str ), unset_char, set_char );
		}

		/// @brief Resize and allocate space for size bits, will grow or shrink size
		/// @details Resizing to smaller does not free allocated memory
		/// @param size Number of bits to allocate space for
		/// @return This set
		constexpr dynamic_bitset& resize( const std::size_t size )
		{
			const std::size_t num_values = ceil_divide( size, base::bits_per_value );
			base::m_container.resize( num_values );
			m_size = size;
			return *this;
		}

		constexpr dynamic_bitset& shrink_to_fit()
		{
			base::m_container.shrink_to_fit();
			return *this;
		}

		[[nodiscard]] constexpr bool operator==( const dynamic_bitset& other ) const noexcept = default;

	private:
		[[nodiscard]] constexpr std::size_t derived_size() const noexcept
		{
			return m_size;
		}

		[[nodiscard]] constexpr underlying_type derived_get_last_mask() const noexcept
		{
			return ( base::one << ( m_size % base::bits_per_value ) ) - 1;
		}

		constexpr void derived_trim() noexcept
		{
			const std::size_t last_bits_used = m_size % base::bits_per_value;
			if ( last_bits_used != 0 )
			{
				const underlying_type last_mask = ( base::one << last_bits_used ) - 1;
				base::m_container.back() &= last_mask;
			}
		}

		std::size_t m_size = 0;
	};
}

namespace std
{
	template <std::unsigned_integral UnderlyingType, typename Allocator>
	struct hash<mclo::dynamic_bitset<UnderlyingType, Allocator>>
		: mclo::hash<mclo::dynamic_bitset<UnderlyingType, Allocator>>
	{
	};
}
