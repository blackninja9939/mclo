#pragma once

#include "mclo/container/span.hpp"
#include "mclo/debug/assert.hpp"
#include "mclo/numeric/math.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <algorithm>
#include <bit>
#include <climits>
#include <compare>
#include <cstddef>
#include <cstring>
#include <limits>
#include <numeric>

namespace mclo::detail
{
	/// @brief CRTP base providing packed integer access algorithms
	/// @details Implements all bit-manipulation operations (get, set, exchange, fill, for_each)
	/// on top of a Derived type that provides physical storage access.
	/// @tparam BitWidth Number of bits per virtual integer, must be in range [1, bits per UnderlyingType]
	/// @tparam UnderlyingType Unsigned integer type used for physical storage
	/// @tparam SizeType The size type used for indexing (typically from the underlying container)
	/// @tparam Derived The concrete packed integer type (CRTP), must implement:
	/// - constexpr underlying_type* derived_data() noexcept;
	/// - constexpr const underlying_type* derived_data() const noexcept;
	/// - constexpr size_type derived_size() const noexcept;           (virtual element count)
	/// - constexpr size_type derived_physical_size() const noexcept;  (physical element count)
	template <std::size_t BitWidth, std::unsigned_integral UnderlyingType, typename SizeType, typename Derived>
	class packed_int_base
	{
		static constexpr std::size_t bits_per_underlying = sizeof( UnderlyingType ) * CHAR_BIT;
		static constexpr UnderlyingType mask = static_cast<UnderlyingType>(
			std::numeric_limits<UnderlyingType>::max() >> ( bits_per_underlying - BitWidth ) );

		// Branchless single unaligned load get() at byte_offset = bit_offset / 8, then shift right by
		// sub_bit = bit_offset % 8. Valid when the loaded sizeof(UnderlyingType) window always
		// contains all BitWidth bits, even at worst-case sub_bit == 7 (the max of bit_offset % 8).
		// Condition: BitWidth + 7 <= bits_per_underlying.
		static constexpr bool use_byte_offset_load =
			std::endian::native == std::endian::little && ( BitWidth + CHAR_BIT - 1 ) <= bits_per_underlying;
		static constexpr bool values_cross_boundaries = ( bits_per_underlying % BitWidth ) != 0;

		// fill() constants
		static constexpr std::size_t fill_gcd = std::gcd( BitWidth, bits_per_underlying );
		static constexpr std::size_t fill_pattern_length = BitWidth / fill_gcd;
		static constexpr std::size_t fill_values_per_pattern = bits_per_underlying / fill_gcd;

		// for_each() constants (only meaningful when !values_cross_boundaries)
		static constexpr SizeType values_per_physical = bits_per_underlying / BitWidth;

		constexpr Derived& as_derived() noexcept
		{
			return static_cast<Derived&>( *this );
		}
		constexpr const Derived& as_derived() const noexcept
		{
			return static_cast<const Derived&>( *this );
		}

		[[nodiscard]] constexpr const UnderlyingType* derived_data() const noexcept
		{
			return as_derived().derived_data();
		}

	public:
		static_assert( BitWidth > 0, "BitWidth must be at least 1" );
		static_assert( BitWidth <= bits_per_underlying,
					   "BitWidth must not exceed the number of bits in UnderlyingType" );

		using value_type = uint_least_t<BitWidth>;
		using size_type = SizeType;
		using underlying_type = UnderlyingType;

		static constexpr std::size_t bit_width = BitWidth;
		static constexpr value_type max_value = mask;

	protected:
		static constexpr std::size_t padding = use_byte_offset_load ? 1 : 0;

		/// @brief Calculate the number of physical elements needed to store a given number of virtual elements
		/// @param virtual_size Number of virtual integers to store
		/// @return Number of physical underlying_type elements required (including padding)
		[[nodiscard]] static constexpr size_type required_physical_size( const size_type virtual_size ) noexcept
		{
			const std::size_t total_bits = virtual_size * bit_width;
			const auto physical = static_cast<size_type>( mclo::ceil_divide( total_bits, bits_per_underlying ) );
			return physical > 0 ? physical + static_cast<size_type>( padding ) : size_type{ 0 };
		}

	public:
		/// @brief Get the virtual integer at the given index
		/// @param index Index of the virtual integer, must be less than size()
		/// @return The value of the virtual integer
		[[nodiscard]] constexpr value_type get( const size_type index ) const noexcept
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );

			const std::size_t bit_offset = index * bit_width;

			if constexpr ( use_byte_offset_load )
			{
				if ( std::is_constant_evaluated() )
				{
					return get_aligned( bit_offset );
				}
				else
				{
					const std::size_t byte_offset = bit_offset / CHAR_BIT;
					const std::size_t sub_bit = bit_offset % CHAR_BIT;

					const auto byte_ptr = reinterpret_cast<const std::byte*>( as_derived().derived_data() );
					underlying_type raw{};
					std::memcpy( &raw, byte_ptr + byte_offset, sizeof( underlying_type ) );

					return static_cast<value_type>( ( raw >> sub_bit ) & mask );
				}
			}
			else
			{
				return get_aligned( bit_offset );
			}
		}

		/// @brief Set the virtual integer at the given index to the given value
		/// @param index Index of the virtual integer, must be less than size()
		/// @param value Value to set, only the lowest BitWidth bits are used
		constexpr void set( const size_type index, const value_type value ) noexcept
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );
			DEBUG_ASSERT( value <= mask, "Value exceeds maximum for BitWidth" );

			const std::size_t bit_offset = index * bit_width;
			const auto physical_index = static_cast<size_type>( bit_offset / bits_per_underlying );
			const std::size_t bit_index = bit_offset % bits_per_underlying;

			auto* data = as_derived().derived_data();
			auto& physical = data[ physical_index ];
			physical &= static_cast<underlying_type>( ~( mask << bit_index ) );
			physical |= static_cast<underlying_type>( static_cast<underlying_type>( value ) << bit_index );

			if constexpr ( values_cross_boundaries )
			{
				const std::size_t bits_in_first = bits_per_underlying - bit_index;
				if ( bits_in_first < bit_width )
				{
					DEBUG_ASSERT( physical_index + 1 < as_derived().derived_physical_size(),
								  "Physical index out of bounds on boundary crossing" );
					const std::size_t remaining_bits = bit_width - bits_in_first;
					auto& next_physical = data[ physical_index + 1 ];
					next_physical &=
						static_cast<underlying_type>( ~( ( underlying_type{ 1 } << remaining_bits ) - 1 ) );
					next_physical |=
						static_cast<underlying_type>( static_cast<underlying_type>( value ) >> bits_in_first );
				}
			}
		}

		/// @brief Get and replace the virtual integer at the given index
		/// @param index Index of the virtual integer, must be less than size()
		/// @param value New value to set, only the lowest BitWidth bits are used
		/// @return The previous value at the given index
		[[nodiscard]] constexpr value_type exchange( const size_type index, const value_type value ) noexcept
		{
			DEBUG_ASSERT( index < size(), "Index out of range" );
			DEBUG_ASSERT( value <= mask, "Value exceeds maximum for BitWidth" );

			const std::size_t bit_offset = index * bit_width;
			const auto physical_index = static_cast<size_type>( bit_offset / bits_per_underlying );
			const std::size_t bit_index = bit_offset % bits_per_underlying;

			auto* data = as_derived().derived_data();
			auto& physical = data[ physical_index ];

			underlying_type old_val = static_cast<underlying_type>( physical >> bit_index );

			physical &= static_cast<underlying_type>( ~( mask << bit_index ) );
			physical |= static_cast<underlying_type>( static_cast<underlying_type>( value ) << bit_index );

			if constexpr ( values_cross_boundaries )
			{
				const std::size_t bits_in_first = bits_per_underlying - bit_index;
				if ( bits_in_first < bit_width )
				{
					DEBUG_ASSERT( physical_index + 1 < as_derived().derived_physical_size(),
								  "Physical index out of bounds on boundary crossing" );
					auto& next_physical = data[ physical_index + 1 ];

					old_val |= static_cast<underlying_type>( next_physical << bits_in_first );

					const std::size_t remaining_bits = bit_width - bits_in_first;
					next_physical &=
						static_cast<underlying_type>( ~( ( underlying_type{ 1 } << remaining_bits ) - 1 ) );
					next_physical |=
						static_cast<underlying_type>( static_cast<underlying_type>( value ) >> bits_in_first );
				}
			}

			return static_cast<value_type>( old_val & mask );
		}

		/// @brief Get the first virtual integer
		[[nodiscard]] constexpr value_type front() const noexcept
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return static_cast<value_type>( as_derived().derived_data()[ 0 ] & mask );
		}

		/// @brief Get the last virtual integer
		[[nodiscard]] constexpr value_type back() const noexcept
		{
			DEBUG_ASSERT( !empty(), "Container is empty" );
			return get( size() - 1 );
		}

		/// @brief Invoke a function for each virtual integer in order
		/// @details When values do not cross physical boundaries, processes multiple values per
		/// physical load with a fully-unrolled inner loop. Otherwise delegates to get().
		/// @param func Callable invoked as func(value_type) for each element
		template <typename Func>
		constexpr void for_each( Func func ) const
		{
			if constexpr ( values_cross_boundaries )
			{
				const size_type sz = size();
				for ( size_type i = 0; i < sz; ++i )
				{
					func( get( i ) );
				}
			}
			else
			{
				const underlying_type* ptr = as_derived().derived_data();
				size_type remaining = size();

				while ( remaining >= values_per_physical )
				{
					const underlying_type current = *ptr++;
					for ( size_type j = 0; j < values_per_physical; ++j )
					{
						func( static_cast<value_type>( ( current >> ( j * bit_width ) ) & mask ) );
					}
					remaining -= values_per_physical;
				}

				if ( remaining > 0 )
				{
					const underlying_type current = *ptr;
					for ( size_type j = 0; j < remaining; ++j )
					{
						func( static_cast<value_type>( ( current >> ( j * bit_width ) ) & mask ) );
					}
				}
			}
		}

		/// @brief Fill all virtual integers with the given value
		/// @param value Value to fill with, only the lowest BitWidth bits are used
		constexpr void fill( const value_type value ) noexcept
		{
			DEBUG_ASSERT( value <= mask, "Value exceeds maximum for BitWidth" );

			const size_type physical_size = as_derived().derived_physical_size();
			if ( physical_size == 0 )
			{
				return;
			}

			underlying_type* const data = as_derived().derived_data();

			underlying_type pattern[ fill_pattern_length ]{};
			const auto uvalue = static_cast<underlying_type>( value );

			std::size_t bit_pos = 0;
			for ( std::size_t i = 0; i < fill_values_per_pattern; ++i )
			{
				const std::size_t physical_index = bit_pos / bits_per_underlying;
				const std::size_t bit_index = bit_pos % bits_per_underlying;

				pattern[ physical_index ] |= static_cast<underlying_type>( uvalue << bit_index );

				const std::size_t bits_in_first = bits_per_underlying - bit_index;
				if ( bits_in_first < bit_width )
				{
					pattern[ physical_index + 1 ] |= static_cast<underlying_type>( uvalue >> bits_in_first );
				}

				bit_pos += bit_width;
			}

			for ( size_type i = 0; i < physical_size; ++i )
			{
				data[ i ] = pattern[ i % fill_pattern_length ];
			}
		}

		/// @brief Get the number of virtual integers
		[[nodiscard]] constexpr size_type size() const noexcept
		{
			return as_derived().derived_size();
		}

		/// @brief Check if the container has no virtual integers
		[[nodiscard]] constexpr bool empty() const noexcept
		{
			return size() == 0;
		}

		/// @brief Get a view of the underlying physical storage
		[[nodiscard]] constexpr mclo::span<const underlying_type> underlying() const noexcept
		{
			return { as_derived().derived_data(), as_derived().derived_physical_size() };
		}

		/// @brief Get a mutable view of the underlying physical storage
		[[nodiscard]] constexpr mclo::span<underlying_type> underlying() noexcept
		{
			return { as_derived().derived_data(), as_derived().derived_physical_size() };
		}

		/// @brief Compare two packed integer containers for equality
		/// @details Compares fully-occupied physical elements directly, then falls back to
		/// element-wise get() only for logical elements touching the last partial physical element
		/// (whose trailing bits may be dirty after resize-down).
		[[nodiscard]] friend constexpr bool operator==( const Derived& lhs, const Derived& rhs ) noexcept
		{
			const size_type sz = lhs.size();
			if ( sz != rhs.size() )
			{
				return false;
			}

			const std::size_t total_bits = static_cast<std::size_t>( sz ) * BitWidth;
			const std::size_t full_physical = total_bits / bits_per_underlying;

			// Compare underlying elements that are fully occupied by logical values
			const auto* lhs_data = static_cast<const packed_int_base&>( lhs ).derived_data();
			const auto* rhs_data = static_cast<const packed_int_base&>( rhs ).derived_data();
			if ( !std::equal( lhs_data, lhs_data + full_physical, rhs_data ) )
			{
				return false;
			}

			// Compare remaining elements that touch the last partial physical element
			const auto tail_start = static_cast<size_type>( full_physical * bits_per_underlying / BitWidth );
			for ( size_type i = tail_start; i < sz; ++i )
			{
				if ( lhs.get( i ) != rhs.get( i ) )
				{
					return false;
				}
			}
			return true;
		}

		/// @brief Three-way compare two packed integer containers lexicographically by virtual element
		/// @details Uses std::mismatch on physical storage to skip over equal regions, then
		/// falls back to element-wise get() only from the first differing physical element onward.
		/// Raw physical comparison cannot determine ordering (lower logical indices occupy less
		/// significant bits), but matching physical elements guarantee matching logical elements.
		[[nodiscard]] friend constexpr std::strong_ordering operator<=>( const Derived& lhs,
																		 const Derived& rhs ) noexcept
		{
			const size_type common = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
			const std::size_t common_bits = static_cast<std::size_t>( common ) * BitWidth;
			const std::size_t common_full_physical = common_bits / bits_per_underlying;

			// Skip over equal physical elements
			const auto* lhs_data = static_cast<const packed_int_base&>( lhs ).derived_data();
			const auto* rhs_data = static_cast<const packed_int_base&>( rhs ).derived_data();
			const auto [ lhs_it, rhs_it ] = std::mismatch( lhs_data, lhs_data + common_full_physical, rhs_data );

			// Compare logical elements from the first differing physical element onward
			const auto physical_offset = static_cast<std::size_t>( lhs_it - lhs_data );
			const auto logical_start = static_cast<size_type>( physical_offset * bits_per_underlying / BitWidth );
			for ( size_type i = logical_start; i < common; ++i )
			{
				const auto cmp = lhs.get( i ) <=> rhs.get( i );
				if ( cmp != 0 )
				{
					return cmp;
				}
			}
			return lhs.size() <=> rhs.size();
		}

	private:
		/// @brief Aligned element access for get(), also used as constexpr fallback
		[[nodiscard]] constexpr value_type get_aligned( const std::size_t bit_offset ) const noexcept
		{
			const auto physical_index = static_cast<size_type>( bit_offset / bits_per_underlying );
			const std::size_t bit_index = bit_offset % bits_per_underlying;

			const auto* data = as_derived().derived_data();
			underlying_type value = static_cast<underlying_type>( data[ physical_index ] >> bit_index );

			if constexpr ( values_cross_boundaries )
			{
				const std::size_t bits_in_first = bits_per_underlying - bit_index;
				if ( bits_in_first < BitWidth )
				{
					DEBUG_ASSERT( physical_index + 1 < as_derived().derived_physical_size(),
								  "Physical index out of bounds on boundary crossing" );
					value |= static_cast<underlying_type>( data[ physical_index + 1 ] << bits_in_first );
				}
			}

			return static_cast<value_type>( value & mask );
		}
	};
}
