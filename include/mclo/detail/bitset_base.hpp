#pragma once

#include "mclo/hash/hash.hpp"
#include "mclo/math.hpp"
#include "mclo/platform.hpp"

#include <bit>
#include <climits>
#include <concepts>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mclo::detail
{
	template <typename Derived, typename UnderlyingContainer>
	class bitset_base
	{
	public:
#ifdef __cpp_lib_ranges
		static_assert( std::contiguous_iterator<typename UnderlyingContainer::iterator>,
					   "UnderlyingContainer must be contiguous in memory" );
#endif

		using underlying_container = UnderlyingContainer;
		using underlying_type = typename underlying_container::value_type;
		static constexpr std::size_t npos = static_cast<std::size_t>( -1 );

	private:
		constexpr Derived& as_derived() noexcept
		{
			return static_cast<Derived&>( *this );
		}
		constexpr const Derived& as_derived() const noexcept
		{
			return static_cast<const Derived&>( *this );
		}

		[[nodiscard]] constexpr underlying_type get_last_mask() const noexcept
		{
			return as_derived().derived_get_last_mask();
		}

	protected:
		static constexpr std::size_t bits_per_value = CHAR_BIT * sizeof( underlying_type );

		static constexpr underlying_type zero = underlying_type{ 0 };
		static constexpr underlying_type one = underlying_type{ 1 };

		template <typename CharT, typename Traits>
		constexpr void init_from_string( std::basic_string_view<CharT, Traits> str,
										 const CharT unset_char,
										 const CharT set_char )
		{
			if ( const std::size_t max_size = this->size(); str.size() > max_size )
			{
				for ( std::size_t index = max_size; index < str.size(); ++index )
				{
					const CharT c = str[ index ];
					if ( !Traits::eq( c, unset_char ) && !Traits::eq( c, set_char ) )
					{
						throw std::invalid_argument( "invalid bitset char" );
					}
				}
				str = str.substr( 0, max_size );
			}

			std::size_t page = 0;
			std::size_t index_in_underlying = 0;
			underlying_type current = zero;

			for ( auto first = str.crbegin(), last = str.crend(); first != last; ++first )
			{
				const CharT c = *first;
				const bool is_set = Traits::eq( c, set_char );

				if ( !is_set && !Traits::eq( c, unset_char ) )
				{
					throw std::invalid_argument( "invalid bitset char" );
				}

				current |= static_cast<underlying_type>( is_set ) << index_in_underlying;

				if ( ++index_in_underlying == bits_per_value )
				{
					m_container[ page++ ] = current;
					current = zero;
					index_in_underlying = 0;
				}
			}

			if ( index_in_underlying != 0 )
			{
				m_container[ page ] = current;
			}
		}

	public:
		constexpr bitset_base() noexcept = default;

		constexpr bitset_base( const underlying_container& container ) noexcept(
			std::is_nothrow_copy_constructible_v<underlying_container> )
			: m_container( container )
		{
			trim();
		}

		constexpr bitset_base( underlying_container&& container ) noexcept(
			std::is_nothrow_move_constructible_v<underlying_container> )
			: m_container( std::move( container ) )
		{
			trim();
		}

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
			const std::size_t end = m_container.size() - static_cast<std::size_t>( last_mask != 0 );
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
			const std::size_t end = m_container.size();
			const std::size_t start_page = start_pos / bits_per_value;
			std::size_t start_index = start_pos % bits_per_value;

			for ( std::size_t page = start_page; page < end; ++page )
			{
				const int bit_index =
					std::countr_zero( static_cast<underlying_type>( m_container[ page ] >> start_index ) );
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
			const std::size_t end = m_container.size() - static_cast<std::size_t>( last_mask != 0 );

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
			for ( std::size_t page = 0, end = m_container.size(); page < end; ++page )
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

		[[nodiscard]] constexpr bool operator==( const bitset_base& other ) const noexcept
		{
			if ( std::is_constant_evaluated() )
			{
				for ( std::size_t page = 0, size = m_container.size(); page < size; ++page )
				{
					if ( m_container[ page ] != other.m_container[ page ] )
					{
						return false;
					}
				}
				return true;
			}
			else
			{
				return std::memcmp( m_container.data(),
									other.m_container.data(),
									m_container.size() * sizeof( underlying_type ) ) == 0;
			}
		}

		constexpr Derived& operator&=( const Derived& other ) noexcept
		{
			for ( std::size_t page = 0, size = m_container.size(); page < size; ++page )
			{
				m_container[ page ] &= other.m_container[ page ];
			}
			return as_derived();
		}

		[[nodiscard]] friend constexpr Derived operator&( const Derived& lhs, const Derived& rhs ) noexcept
		{
			Derived result = lhs;
			result &= rhs;
			return result;
		}

		constexpr Derived& operator|=( const Derived& other ) noexcept
		{
			for ( std::size_t page = 0, size = m_container.size(); page < size; ++page )
			{
				m_container[ page ] |= other.m_container[ page ];
			}
			return as_derived();
		}

		[[nodiscard]] friend constexpr Derived operator|( const Derived& lhs, const Derived& rhs ) noexcept
		{
			Derived result = lhs;
			result |= rhs;
			return result;
		}

		constexpr Derived& operator^=( const Derived& other ) noexcept
		{
			for ( std::size_t page = 0, size = m_container.size(); page < size; ++page )
			{
				m_container[ page ] ^= other.m_container[ page ];
			}
			return as_derived();
		}

		[[nodiscard]] friend constexpr Derived operator^( const Derived& lhs, const Derived& rhs ) noexcept
		{
			Derived result = lhs;
			result ^= rhs;
			return result;
		}

		[[nodiscard]] constexpr Derived operator~() const noexcept( std::is_nothrow_copy_constructible_v<Derived> )
		{
			return Derived( as_derived() ).flip();
		}

		constexpr Derived& operator<<=( std::size_t pos ) noexcept
		{
			const auto size = static_cast<std::ptrdiff_t>( m_container.size() - 1 );
			const auto value_shift = static_cast<std::ptrdiff_t>( pos / bits_per_value );

			// Shift whole values
			if ( value_shift != 0 )
			{
				for ( std::ptrdiff_t index = size; 0 <= index; --index )
				{
					m_container[ index ] = value_shift <= index ? m_container[ index - value_shift ] : 0;
				}
			}

			// Shift by bits
			if ( ( pos %= bits_per_value ) != 0 )
			{
				for ( std::ptrdiff_t index = size; 0 < index; --index )
				{
					m_container[ index ] =
						( m_container[ index ] << pos ) | ( m_container[ index - 1 ] >> ( bits_per_value - pos ) );
				}

				m_container[ 0 ] <<= pos;
			}

			trim();
			return as_derived();
		}

		constexpr Derived& operator>>=( std::size_t pos ) noexcept
		{
			const auto size = static_cast<std::ptrdiff_t>( m_container.size() - 1 );
			const auto value_shift = static_cast<std::ptrdiff_t>( pos / bits_per_value );

			// Shift whole values
			if ( value_shift != 0 )
			{
				for ( std::ptrdiff_t index = 0; index <= size; ++index )
				{
					m_container[ index ] = value_shift <= size - index ? m_container[ index + value_shift ] : 0;
				}
			}

			// Shift by bits
			if ( ( pos %= bits_per_value ) != 0 )
			{
				for ( std::ptrdiff_t index = 0; index < size; ++index )
				{
					m_container[ index ] =
						( m_container[ index ] >> pos ) | ( m_container[ index + 1 ] << ( bits_per_value - pos ) );
				}

				m_container[ size ] >>= pos;
			}

			return as_derived();
		}

		[[nodiscard]] constexpr Derived operator<<( const std::size_t pos ) const noexcept
		{
			Derived copy( as_derived() );
			copy <<= pos;
			return copy;
		}

		[[nodiscard]] constexpr Derived operator>>( const std::size_t pos ) const noexcept
		{
			Derived copy( as_derived() );
			copy >>= pos;
			return copy;
		}

		template <typename CharT = char,
				  typename Traits = std::char_traits<CharT>,
				  typename Allocator = std::allocator<CharT>>
		constexpr std::basic_string<CharT, Traits, Allocator> to_string( CharT unset_char = CharT( '0' ),
																		 CharT set_char = CharT( '1' ) ) const
		{
			const std::size_t length = size();
			std::basic_string<CharT, Traits, Allocator> result( length, unset_char );
			for_each_set( [ & ]( const std::size_t index ) { result[ length - 1 - index ] = set_char; } );
			return result;
		}

		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const Derived& value ) noexcept
		{
			hasher.write( std::as_bytes( value.underlying() ) );
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
}
