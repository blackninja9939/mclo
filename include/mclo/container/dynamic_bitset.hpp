
#pragma once

#include "mclo/container/detail/bitset_base.hpp"

#include <vector>

namespace mclo
{
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

		explicit dynamic_bitset( const std::size_t size )
		{
			resize( size );
		}

		template <typename StringLike, typename CharT = typename StringLike::value_type>
		constexpr explicit dynamic_bitset( const StringLike& str,
										   const CharT unset_char = CharT( '0' ),
										   const CharT set_char = CharT( '1' ) )
			: dynamic_bitset( str.size() )
		{
			using view = std::basic_string_view<CharT, typename StringLike::traits_type>;
			base::init_from_string( view( str ), unset_char, set_char );
		}

		constexpr dynamic_bitset& resize( const std::size_t size )
		{
			const std::size_t num_values = ceil_divide( size, base::bits_per_value );
			base::m_container.resize( num_values );
			m_size = size;
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
	{
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR std::size_t operator()(
			const mclo::dynamic_bitset<UnderlyingType, Allocator>& bitset ) MCLO_CONST_CALL_OPERATOR noexcept
		{
			return mclo::hash( bitset );
		}
	};
}
