#pragma once

#include "mclo/math.hpp"
#include "mclo/standard_integer_type.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <iterator>
#include <span>
#include <vector>

namespace mclo
{
	template <typename Derived, typename UnderlyingContainer>
	class bitset_base
	{
	public:
		static_assert( std::contiguous_iterator<typename UnderlyingContainer::iterator>,
					   "UnderlyingContainer must be contiguous in memory" );

		using underlying_container = UnderlyingContainer;
		using underlying_type = typename underlying_container::value_type;
		static constexpr std::size_t npos = static_cast<std::size_t>( -1 );

	private:
		[[nodiscard]] constexpr Derived& as_derived() noexcept
		{
			return static_cast<Derived&>( *this );
		}
		[[nodiscard]] constexpr const Derived& as_derived() const noexcept
		{
			return static_cast<const Derived&>( *this );
		}

		[[nodiscard]] constexpr std::size_t num_values() const noexcept
		{
			return as_derived().derived_num_values();
		}
		[[nodiscard]] constexpr underlying_type get_last_mask() const noexcept
		{
			return as_derived().derived_get_last_mask();
		}

	protected:
		static constexpr std::size_t bits_per_value = CHAR_BIT * sizeof( underlying_type );

		static constexpr underlying_type zero = underlying_type{ 0 };
		static constexpr underlying_type one = underlying_type{ 1 };

	public:
		constexpr bitset_base() noexcept = default;

		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return as_derived().derived_size();
		}

		[[nodiscard]] constexpr bool test( const std::size_t pos ) const noexcept
		{
			const std::size_t page = pos / bits_per_value;
			const underlying_type bit_value = one << ( pos % bits_per_value );
			return ( m_container[ page ] & bit_value ) != 0;
		}

		[[nodiscard]] constexpr bool test_set( const std::size_t pos, const bool value = true ) noexcept
		{
			assert( pos < size() );
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
			const underlying_type last_mask = get_last_mask();
			const std::size_t end = num_values() - static_cast<std::size_t>( last_mask != 0 );
			for ( std::size_t index = 0; index < end; ++index )
			{
				if ( m_container[ index ] != ~zero )
				{
					return false;
				}
			}
			if ( last_mask != 0 )
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

		constexpr Derived& set() noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				std::fill( m_container.begin(), m_container.end(), static_cast<underlying_type>( -1 ) );
			}
			else
			{
				std::memset( m_container.data(), -1, m_container.size() * sizeof( underlying_type ) );
			}
			trim();
			return as_derived();
		}

		constexpr Derived& set( const std::size_t pos ) noexcept
		{
			return set_internal<true>( pos );
		}

		constexpr Derived& set( const std::size_t pos, const bool value ) noexcept
		{
			return value ? set( pos ) : reset( pos );
		}

		constexpr Derived& reset() noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				std::fill( m_container.begin(), m_container.end(), static_cast<underlying_type>( -1 ) );
			}
			else
			{
				std::memset( m_container.data(), 0, m_container.size() * sizeof( underlying_type ) );
			}
			return as_derived();
		}

		constexpr Derived& reset( const std::size_t pos ) noexcept
		{
			return set_internal<false>( pos );
		}

		constexpr Derived& flip() noexcept
		{
			for ( underlying_type& value : m_container )
			{
				value = ~value;
			}
			trim();
			return as_derived();
		}

		constexpr Derived& flip( const std::size_t pos ) noexcept
		{
			assert( pos < size() );
			const std::size_t page = pos / bits_per_value;
			const std::size_t index = pos % bits_per_value;
			m_container[ page ] ^= one << index;
			return as_derived();
		}

		[[nodiscard]] constexpr std::size_t find_first_set( const std::size_t start_pos = 0 ) const noexcept
		{
			const std::size_t end = num_values();
			const std::size_t start_page = start_pos / bits_per_value;
			std::size_t start_index = start_pos % bits_per_value;

			for ( std::size_t page = start_page; page < end; ++page )
			{
				const int bit_index = std::countr_zero( static_cast<underlying_type>( m_container[ page ] >> start_index ) );
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
			const underlying_type last_mask = get_last_mask();
			const std::size_t end = num_values() - static_cast<std::size_t>( last_mask != 0 );

			const std::size_t start_page = start_pos / bits_per_value;
			const std::size_t start_index = start_pos % bits_per_value;
			underlying_type mask = ( one << start_index ) - 1;

			for ( std::size_t page = start_page; page < end; ++page )
			{
				const int bit_index = std::countr_one( static_cast<underlying_type>( m_container[ page ] | mask ) );
				if ( bit_index != bits_per_value )
				{
					return ( page * bits_per_value ) + bit_index;
				}
				mask = 0; // Moved off of partial page
			}

			if ( last_mask != 0 )
			{
				const int bit_index =
					std::countr_one( static_cast<underlying_type>( m_container.back() | mask | ~last_mask ) );
				if ( bit_index != bits_per_value )
				{
					return ( end * bits_per_value ) + bit_index;
				}
			}

			return npos;
		}

		constexpr void for_each_set( std::invocable<std::size_t> auto func ) const noexcept
		{
			for ( std::size_t page = 0, end = num_values(); page < end; ++page )
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

		[[nodiscard]] constexpr auto underlying() const noexcept
		{
			return std::span( m_container );
		}
		[[nodiscard]] constexpr auto underlying() noexcept
		{
			return std::span( m_container );
		}

	private:
		template <bool value>
		[[nodiscard]] constexpr Derived& set_internal( const std::size_t pos ) noexcept
		{
			assert( pos < size() );
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
			return as_derived();
		}

		constexpr void trim() noexcept
		{
			as_derived().derived_trim();
		}

	protected:
		underlying_container m_container{};
	};

	namespace detail
	{
		template <typename UnderlyingType>
		[[nodiscard]] constexpr std::size_t num_values_for_bits( const std::size_t num_bits ) noexcept
		{
			return mclo::ceil_divide( num_bits, CHAR_BIT * sizeof( UnderlyingType ) );
		}

		template <std::size_t Bits, std::unsigned_integral UnderlyingType>
		using fixed_bitset_storage = std::array<UnderlyingType, num_values_for_bits<UnderlyingType>( Bits )>;
	}

	template <std::size_t Bits, std::unsigned_integral UnderlyingType = mclo::uint_least_t<Bits>>
	class bitset : public bitset_base<bitset<Bits, UnderlyingType>, detail::fixed_bitset_storage<Bits, UnderlyingType>>
	{
		using base = bitset_base<bitset<Bits, UnderlyingType>, detail::fixed_bitset_storage<Bits, UnderlyingType>>;
		friend class base;

	public:
		using underlying_container = base::underlying_container;
		using underlying_type = base::underlying_type;

		[[nodiscard]] constexpr bool operator==( const bitset& other ) const noexcept = default;

	private:
		static constexpr std::size_t num_values = detail::num_values_for_bits<UnderlyingType>( Bits );
		static constexpr bool last_needs_mask = Bits % base::bits_per_value != 0;
		static constexpr UnderlyingType last_mask = ( base::one << ( Bits % base::bits_per_value ) ) - 1;

		[[nodiscard]] static constexpr std::size_t derived_size() noexcept
		{
			return Bits;
		}

		[[nodiscard]] static constexpr std::size_t derived_num_values() noexcept
		{
			return num_values;
		}

		[[nodiscard]] static constexpr underlying_type derived_get_last_mask() noexcept
		{
			return last_mask;
		}

		constexpr void derived_trim() noexcept
		{
			if constexpr ( last_needs_mask )
			{
				base::m_container.back() &= last_mask;
			}
		}
	};

	template <std::unsigned_integral UnderlyingType = std::size_t, typename Allocator = std::allocator<UnderlyingType>>
	class dynamic_bitset
		: public bitset_base<dynamic_bitset<UnderlyingType, Allocator>, std::vector<UnderlyingType, Allocator>>
	{
		using base = bitset_base<dynamic_bitset<UnderlyingType, Allocator>, std::vector<UnderlyingType, Allocator>>;
		friend class base;

	public:
		using underlying_container = base::underlying_container;
		using underlying_type = base::underlying_type;

		constexpr dynamic_bitset() noexcept = default;

		explicit dynamic_bitset( const std::size_t size )
		{
			resize( size );
		}

		constexpr dynamic_bitset& resize( const std::size_t size )
		{
			const std::size_t num_values = mclo::ceil_divide( size, base::bits_per_value );
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

		[[nodiscard]] constexpr std::size_t derived_num_values() const noexcept
		{
			return base::m_container.size();
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
