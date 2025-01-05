#pragma once

#include "mclo/container/detail/bitset_base.hpp"
#include "mclo/hash/std_adapter.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <array>
#include <cinttypes>

namespace mclo
{
	namespace detail
	{
		template <typename UnderlyingType>
		[[nodiscard]] constexpr std::size_t num_values_for_bits( const std::size_t num_bits ) noexcept
		{
			return ceil_divide( num_bits, CHAR_BIT * sizeof( UnderlyingType ) );
		}

		template <std::size_t Bits, std::unsigned_integral UnderlyingType>
		using fixed_bitset_storage = std::array<UnderlyingType, num_values_for_bits<UnderlyingType>( Bits )>;
	}

	/*
	 * Optimized implementation of bitset that also supports an improved API:
	 * - Picks smallest integer type for number of bits by default
	 * - Access to underlying std::array of underlying integer type
	 * - Usable at compile time
	 * - Exception safe
	 * - Supports fast iteration via for_each_set
	 * - Supports fast iteration via find_first_set/unset including starting offset
	 * - test_and_set in one function
	 *
	 * The following std::bitset functionality is divergent:
	 * - No mutable operator[], returns a proxy reference which indirectly calls set, less efficient and less safe
	 * compared to explicitly using set/reset
	 * - Restricted Implicit constructor from integer, only usable if Bits fits into one underlying type, otherwise must
	 * use from std::array constructor
	 * - Restricted to_ulong/ullong, same as above, only usable if number of bits would fit, provides access to the
	 * underlying container instead
	 * - All of the string constructor overloads, I've just done a simple one for string_view since it can convert all
	 * and you can use its substr function for offsets
	 */
	template <std::size_t Bits, std::unsigned_integral UnderlyingType = uint_least_t<Bits>>
	class bitset
		: public detail::bitset_base<bitset<Bits, UnderlyingType>, detail::fixed_bitset_storage<Bits, UnderlyingType>>
	{
		using base =
			detail::bitset_base<bitset<Bits, UnderlyingType>, detail::fixed_bitset_storage<Bits, UnderlyingType>>;
		friend base;

		static constexpr std::size_t num_values = detail::num_values_for_bits<UnderlyingType>( Bits );

	public:
		using underlying_container = typename base::underlying_container;
		using underlying_type = typename base::underlying_type;

		using base::base;

		constexpr bitset( const underlying_type value )
			requires( num_values == 1 )
			: base( std::array{ value } )
		{
		}

		template <typename StringLike, typename CharT = typename StringLike::value_type>
		constexpr explicit bitset( const StringLike& str,
								   const CharT unset_char = CharT( '0' ),
								   const CharT set_char = CharT( '1' ) )
		{
			using view = std::basic_string_view<CharT, typename StringLike::traits_type>;
			base::init_from_string( view( str ), unset_char, set_char );
		}

		[[nodiscard]] constexpr unsigned long to_ulong() const noexcept
			requires( num_values == 1 && sizeof( underlying_type ) <= sizeof( unsigned long ) )
		{
			return static_cast<unsigned long>( base::underlying().front() );
		}

		[[nodiscard]] constexpr unsigned long long to_ullong() const noexcept
			requires( num_values == 1 && sizeof( underlying_type ) <= sizeof( unsigned long long ) )
		{
			return static_cast<unsigned long long>( base::underlying().front() );
		}

		[[nodiscard]] constexpr bool operator[]( const std::size_t index ) const noexcept
		{
			return base::test( index );
		}

		[[nodiscard]] constexpr bool operator==( const bitset& other ) const noexcept = default;

	private:
		static constexpr bool last_needs_mask = Bits % base::bits_per_value != 0;
		static constexpr UnderlyingType last_mask = ( base::one << ( Bits % base::bits_per_value ) ) - 1;

		[[nodiscard]] static constexpr std::size_t derived_size() noexcept
		{
			return Bits;
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
}

namespace std
{
	template <std::size_t Bits, std::unsigned_integral UnderlyingType>
	struct hash<mclo::bitset<Bits, UnderlyingType>> : mclo::std_hash_adapter<mclo::bitset<Bits, UnderlyingType>>
	{
	};
}
