#pragma once

#include "mclo/bit.hpp"
#include "mclo/math.hpp"
#include "mclo/standard_integer_type.hpp"

#include <array>
#include <concepts>

namespace mclo
{
	template <std::size_t Bits, std::unsigned_integral UnderlyingType = mclo::uint_least_t<Bits>>
	class bitset
	{
		static_assert( Bits > 0, "Zero sized bitsets are not supported" );

		static constexpr std::size_t bits_per_value = CHAR_BIT * sizeof( UnderlyingType );
		static constexpr std::size_t num_values = mclo::ceil_divide( Bits, bits_per_value );

		static constexpr UnderlyingType zero = UnderlyingType{ 0 };
		static constexpr UnderlyingType one = UnderlyingType{ 1 };

		static constexpr bool last_needs_mask = Bits % bits_per_value != 0;
		static constexpr UnderlyingType last_mask = ( one << ( Bits % bits_per_value ) ) - 1;

	public:
		using underlying_type = UnderlyingType;
		using underlying_container = std::array<underlying_type, num_values>;
		static constexpr std::size_t npos = static_cast<std::size_t>( -1 );

		constexpr bitset() noexcept = default;

		[[nodiscard]] constexpr bool operator==( const bitset& other ) const noexcept = default;

		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return Bits;
		}

		[[nodiscard]] constexpr bool test( const std::size_t pos ) const noexcept
		{
			return ( m_container[ pos / bits_per_value ] & ( one << pos % bits_per_value ) ) != 0;
		}

		[[nodiscard]] constexpr bool test_set( const std::size_t pos, const bool value = true ) noexcept
		{
			assert( pos < Bits );
			const std::size_t page = pos / bits_per_value;
			const underlying_type bit_value = one << ( pos % bits_per_value );
			underlying_type& data = m_container[ page ];

			const bool old_value = data & bit_value;

			if ( value )
			{
				data |= bit_value;
			}
			else
			{
				data &= ~bit_value;
			}

			return old_value;
		}

		[[nodiscard]] constexpr bool all() const noexcept
		{
			constexpr std::size_t end = num_values - static_cast<std::size_t>( last_needs_mask );
			for ( std::size_t index = 0; index < end; ++index )
			{
				if ( m_container[ index ] != ~zero )
				{
					return false;
				}
			}
			if constexpr ( last_needs_mask )
			{
				return m_container.back() == last_mask;
			}
			else
			{
				return true;
			}
		}

		[[nodiscard]] constexpr bool any() const noexcept
		{
			for ( const underlying_type value : m_container )
			{
				if ( value != 0 )
				{
					return true;
				}
			}
			return false;
		}

		[[nodiscard]] constexpr bool none() const noexcept
		{
			return !any();
		}

		[[nodiscard]] constexpr std::size_t count() const noexcept
		{
			std::size_t total_set = 0;
			for ( const underlying_type value : m_container )
			{
				total_set += std::popcount( value );
			}
			return total_set;
		}

		constexpr bitset& set() noexcept
		{
			m_container.fill( static_cast<underlying_type>( -1 ) );
			trim();
			return *this;
		}

		constexpr bitset& set( const std::size_t pos ) noexcept
		{
			return set_internal<true>( pos );
		}

		constexpr bitset& set( const std::size_t pos, const bool value ) noexcept
		{
			return value ? set( pos ) : reset( pos );
		}

		[[nodiscard]] constexpr bitset& reset() noexcept
		{
			m_container.fill( 0 );
			return *this;
		}

		constexpr bitset& reset( const std::size_t pos ) noexcept
		{
			return set_internal<false>( pos );
		}

		constexpr bitset& flip() noexcept
		{
			for ( underlying_type& value : m_container )
			{
				value = ~value;
			}
			trim();
			return *this;
		}

		constexpr bitset& flip( const std::size_t pos ) noexcept
		{
			assert( pos < Bits );
			const std::size_t page = pos / bits_per_value;
			const std::size_t index = pos % bits_per_value;
			m_container[ page ] ^= one << index;
		}

		[[nodiscard]] constexpr std::size_t find_first_set( const std::size_t start_pos = 0 ) const noexcept
		{
			const std::size_t start_page = start_pos / bits_per_value;
			std::size_t start_index = start_pos % bits_per_value;

			for ( std::size_t page = start_page; page < num_values; ++page )
			{
				const int bit_index = std::countr_zero( m_container[ page ] >> start_index );
				if ( bit_index != bits_per_value )
				{
					return ( page * bits_per_value ) + start_index + bit_index;
				}
				start_index = 0; // Moved off of partial page, no need to adjust now
			}

			return npos;
		}

		[[nodiscard]] constexpr std::size_t find_first_unset( const std::size_t start_pos = 0 ) const noexcept
		{
			constexpr std::size_t end = num_values - static_cast<std::size_t>( last_needs_mask );

			const std::size_t start_page = start_pos / bits_per_value;
			const std::size_t start_index = start_pos % bits_per_value;
			underlying_type mask = ( one << start_index ) - 1;

			for ( std::size_t page = start_page; page < end; ++page )
			{
				const int bit_index = std::countr_one( m_container[ page ] | mask );
				if ( bit_index != bits_per_value )
				{
					return ( page * bits_per_value ) + bit_index;
				}
				mask = 0; // Moved off of partial page
			}

			if constexpr ( last_needs_mask )
			{
				const int bit_index = std::countr_one( m_container.back() | mask | ~last_mask );
				if ( bit_index != bits_per_value )
				{
					return ( ( num_values - 1 ) * bits_per_value ) + bit_index;
				}
			}

			return npos;
		}

		constexpr void for_each_set( std::invocable<std::size_t> auto func ) const noexcept
		{
			for ( std::size_t page = 0; page < num_values; ++page )
			{
				underlying_type value = m_container[ page ];
				while ( value )
				{
					const int bit_index = std::countr_zero( value );
					func( ( page * bits_per_value ) + bit_index );
					value &= value - 1; // Clear rightmost set bit
				}
			}
		}

		[[nodiscard]] constexpr underlying_container& underlying() noexcept
		{
			return m_container;
		}
		[[nodiscard]] constexpr const underlying_container& underlying() const noexcept
		{
			return m_container;
		}

	private:
		template <bool value>
		[[nodiscard]] constexpr bitset& set_internal( const std::size_t pos ) noexcept
		{
			assert( pos < Bits );
			const std::size_t page = pos / bits_per_value;
			const underlying_type bit_value = one << ( pos % bits_per_value );
			underlying_type& data = m_container[ page ];
			if constexpr ( value )
			{
				data |= bit_value;
			}
			else
			{
				data &= ~bit_value;
			}
			return *this;
		}

		constexpr void trim() noexcept
		{
			if constexpr ( last_needs_mask )
			{
				m_container.back() &= last_mask;
			}
		}

		underlying_container m_container{};
	};
}
