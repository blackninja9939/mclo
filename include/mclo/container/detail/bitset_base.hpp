#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/hash/hash_append_range.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <bit>
#include <climits>
#include <concepts>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mclo::detail
{
	/// @brief Optimized implementation of std::bitset with improved API and performance
	/// @details
	/// - Access to container of underlying integer type
	/// - Usable at compile time if container is
	/// - Exception safe querying and modification of the set, only container growth can throw if the container can be
	/// resized
	/// - Supports fast iteration via for_each_set
	/// - Supports fast iteration via find_first_set/unset including starting offset
	/// - test_and_set in one function
	/// @warnings The follow std::bitset functionality is divergent
	/// - No mutable operator[], in std::bitset it returns a proxy reference which indirectly calls set, less efficient
	/// and less safe
	/// compared to explicitly using set/reset
	/// - All of the string constructor overloads, I've just done a simple one for string_view since it can convert
	/// all and you can use its substr function for offsets
	/// @brief Core bitset API implemented ontop of a Derived type implementing the derived API
	/// @tparam Derived Derived type implementing the bitset, must implement:
	/// constexpr std::size_t derived_size() const noexept;
	/// constexpr underlying_type derived_get_last_mask() const noexept;
	/// constexpr void derived_trim() noexept;
	/// @tparam UnderlyingContainer The underlying container storing the bits
	template <typename Derived, std::ranges::contiguous_range UnderlyingContainer>
	class bitset_base
	{
	public:
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
		/// @brief Construct the bitset with no set bits
		constexpr bitset_base() noexcept = default;

		/// @brief Construct the bitset from a copy underlying container
		/// @details The container will trim set bits outside of the maximum size
		/// @param container to copy from
		constexpr bitset_base( const underlying_container& container ) noexcept(
			std::is_nothrow_copy_constructible_v<underlying_container> )
			: m_container( container )
		{
			trim();
		}

		/// @brief Construct the bitset from a moved from underlying container
		/// @details The container will trim set bits outside of the maximum size
		/// @param container to move from
		constexpr bitset_base( underlying_container&& container ) noexcept(
			std::is_nothrow_move_constructible_v<underlying_container> )
			: m_container( std::move( container ) )
		{
			trim();
		}

		/// @brief Gets the total number of bits in the bitset
		/// @return Number of bits
		[[nodiscard]] constexpr std::size_t size() const noexcept
		{
			return as_derived().derived_size();
		}

		/// @brief Test if the bit at pos is set
		/// @warning Do not use in a loop, prefer for_each_set or find_first_set/unset, they are much faster
		/// @param pos Position to check, must be < size()
		/// @return If the bit is set
		[[nodiscard]] constexpr bool test( const std::size_t pos ) const MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos < size(), "Pos out of range of bitset" );
			const std::size_t page = pos / bits_per_value;
			const underlying_type bit_value = one << ( pos % bits_per_value );
			return ( m_container[ page ] & bit_value ) != 0;
		}

		/// @brief Test if the bit at pos is set, then set it to value
		/// @warning Do not use in a loop to test, prefer for_each_set or find_first_set/unset, they are much faster
		/// @param pos Position to check, must be < size()
		/// @param value Value to set the bit
		/// @return If the bit was set
		[[nodiscard]] constexpr bool test_set( const std::size_t pos, const bool value = true ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos < size(), "Pos out of range of bitset" );
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

		/// @brief Check if all bits are set
		/// @return If all are set
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

		/// @brief Check if any bits are set
		/// @return If any are set
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

		/// @brief Check if no bits are set
		/// @return If none are set
		[[nodiscard]] constexpr bool none() const noexcept
		{
			return !any();
		}
		
		/// @brief Count the number of set bits
		/// @return The number of set bits
		[[nodiscard]] constexpr std::size_t count() const noexcept
		{
			std::size_t total_set = 0;
			for ( const underlying_type value : m_container )
			{
				total_set += std::popcount( value );
			}
			return total_set;
		}

		/// @brief Set every bit
		/// @return Reference to the set
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

		/// @brief Set the bit at pos
		/// @param pos Position to set, must be < size()
		/// @return Reference to the set
		constexpr Derived& set( const std::size_t pos ) MCLO_NOEXCEPT_TESTS
		{
			return set_internal<true>( pos );
		}

		/// @brief Set the bit at pos to value
		/// @param pos Position to set, must be < size()
		/// @return Reference to the set
		constexpr Derived& set( const std::size_t pos, const bool value ) noexcept
		{
			return value ? set( pos ) : reset( pos );
		}

		/// @brief Clear every bit in the set
		/// @return Reference to the set
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

		/// @brief Clear the bit at pos
		/// @param pos Position to clear, must be < size()
		/// @return Reference to the set
		constexpr Derived& reset( const std::size_t pos ) MCLO_NOEXCEPT_TESTS
		{
			return set_internal<false>( pos );
		}

		/// @brief Flip ever bit
		/// @return Reference to the set
		constexpr Derived& flip() noexcept
		{
			for ( underlying_type& value : m_container )
			{
				value = ~value;
			}
			trim();
			return as_derived();
		}

		/// @brief Flip the bit at pos
		/// @param pos Position to flip, must be < size()
		/// @return Reference to the set
		constexpr Derived& flip( const std::size_t pos ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos < size(), "Pos out of range of bitset" );
			const std::size_t page = pos / bits_per_value;
			const std::size_t index = pos % bits_per_value;
			m_container[ page ] ^= one << index;
			return as_derived();
		}

		/// @brief Find the first set bit starting from start_pos
		/// @param start_pos Position to start searching from
		/// @return Positon of the first set bit, or npos if none set
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

		/// @brief Find the first unset bit starting from start_pos
		/// @param start_pos Position to start searching from
		/// @return Positon of the first unset bit, or npos if none set
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

		/// @brief Iterate over every set bit and call func with the position
		/// @details Optimized for fast looping vs find_first_set or test loops
		/// @param func Callable that takes the std::size_t position
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

		/// @brief Get a const span over the underlying container
		/// @return Const span of the container
		[[nodiscard]] constexpr auto underlying() const noexcept
		{
			return std::span( m_container );
		}

		/// @brief Get a mutable span over the underlying container
		/// @return Mutable span of the container
		[[nodiscard]] constexpr auto underlying() noexcept
		{
			return std::span( m_container );
		}

		/// @brief Check that all set bits in other are the same as this
		/// @param other Bitset to check against
		/// @return If all bits are the same
		[[nodiscard]] constexpr bool operator==( const bitset_base& other ) const noexcept
		{
			const std::size_t size = m_container.size();
			if ( size != other.m_container.size() )
			{
				return false;
			}
			if ( std::is_constant_evaluated() )
			{
				for ( std::size_t page = 0; page < size; ++page )
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
				return std::memcmp( m_container.data(), other.m_container.data(), size * sizeof( underlying_type ) ) ==
					   0;
			}
		}

		/// @brief Bitwise &= of this set with other, performs &= on every bit
		/// @param other The set to & with
		/// @return This set
		constexpr Derived& operator&=( const Derived& other ) noexcept
		{
			for ( std::size_t page = 0, size = std::min( m_container.size(), other.m_container.size() ); page < size;
				  ++page )
			{
				m_container[ page ] &= other.m_container[ page ];
			}
			return as_derived();
		}

		/// @brief Bitwise & of lhs with rhs, performs & on every bit
		/// @param lhs Left set to &
		/// @param rhs Right set to &
		/// @return New set with the & of every bit
		[[nodiscard]] friend constexpr Derived operator&( const Derived& lhs, const Derived& rhs ) noexcept
		{
			Derived result = lhs;
			result &= rhs;
			return result;
		}

		/// @brief Bitwise |= of this set with other, performs |= on every bit
		/// @param other The set to | with
		/// @return This set
		constexpr Derived& operator|=( const Derived& other ) noexcept
		{
			for ( std::size_t page = 0, size = std::min( m_container.size(), other.m_container.size() ); page < size;
				  ++page )
			{
				m_container[ page ] |= other.m_container[ page ];
			}
			return as_derived();
		}

		/// @brief Bitwise | of lhs with rhs, performs | on every bit
		/// @param lhs Left set to |
		/// @param rhs Right set to |
		/// @return New set with the | of every bit
		[[nodiscard]] friend constexpr Derived operator|( const Derived& lhs, const Derived& rhs ) noexcept
		{
			Derived result = lhs;
			result |= rhs;
			return result;
		}

		/// @brief Bitwise ^= of this set with other, performs ^= on every bit
		/// @param other The set to ^ with
		/// @return This set
		constexpr Derived& operator^=( const Derived& other ) noexcept
		{
			for ( std::size_t page = 0, size = std::min( m_container.size(), other.m_container.size() ); page < size;
				  ++page )
			{
				m_container[ page ] ^= other.m_container[ page ];
			}
			return as_derived();
		}

		/// @brief Bitwise ^ of lhs with rhs, performs & on every bit
		/// @param lhs Left set to ^
		/// @param rhs Right set to ^
		/// @return New set with the ^ of every bit
		[[nodiscard]] friend constexpr Derived operator^( const Derived& lhs, const Derived& rhs ) noexcept
		{
			Derived result = lhs;
			result ^= rhs;
			return result;
		}

		/// @brief Flip every set bit and return the new set
		/// @return new set with the ~ of every bit
		[[nodiscard]] constexpr Derived operator~() const noexcept( std::is_nothrow_copy_constructible_v<Derived> )
		{
			return Derived( as_derived() ).flip();
		}

		/// @brief Left shift this set by pos bits
		/// @param pos Amount to shift by
		/// @return This set
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

		/// @brief Right shift this set by pos bits
		/// @param pos Amount to shift by
		/// @return This set
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

		/// @brief Left shifted copy of the set by pos bits
		/// @param pos Amount to shift by
		/// @return The new shifted set
		[[nodiscard]] constexpr Derived operator<<( const std::size_t pos ) const noexcept
		{
			Derived copy( as_derived() );
			copy <<= pos;
			return copy;
		}

		/// @brief Right shifted copy of the set by pos bits
		/// @param pos Amount to shift by
		/// @return The new shifted set
		[[nodiscard]] constexpr Derived operator>>( const std::size_t pos ) const noexcept
		{
			Derived copy( as_derived() );
			copy >>= pos;
			return copy;
		}

		/// @brief Format the bitset into a std::basic_string
		/// @tparam CharT Character type to format string of, defaults to char
		/// @tparam Traits Character traits for the string, defaults to std::char_traits<CharT>
		/// @tparam Allocator Allocator for the string, defaults to std::allocator<CharT>
		/// @param unset_char Character for unset bits, defaults to CharT( '0' )
		/// @param set_char Character for set bits, defaults to CharT( '1' )
		/// @return std::basic_string<CharT, Traits, Allocator> of size(), with each character being unset_char or
		/// set_char for every position in the set
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

		/// @brief Append hash data for the bitset
		/// @tparam Hasher Type meeting the hasher concept
		/// @param hasher The hasher instance
		/// @param value The bitset to hash
		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const Derived& value ) noexcept
		{
			hash_append_range( hasher, value.underlying() );
		}

	private:
		template <bool value>
		[[nodiscard]] constexpr Derived& set_internal( const std::size_t pos ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( pos < size(), "Pos out of range of bitset" );
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
