#pragma once

#include "mclo/container/detail/bitset_base.hpp"
#include "mclo/hash/hash.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <array>
#include <cinttypes>

namespace mclo
{
	namespace detail
	{
		template <std::size_t Bits, std::unsigned_integral UnderlyingType>
		using fixed_bitset_storage = std::array<UnderlyingType, num_values_for_bits<UnderlyingType>( Bits )>;
	}

	/// @brief Optimized implementation of std::bitset with improved API and performance
	/// @details
	/// - Picks smallest integer type for number of bits by default
	/// - Access to underlying std::array of underlying integer type
	/// @warnings The follow std::bitset functionality is divergent
	/// - Restricted Implicit constructor from integer, only usable if Bits fits into one underlying type, otherwise
	/// must use from std::array constructor
	/// - Restricted to_ulong/ullong, same as above, only usable if number of bits
	/// would fit, provides access to the underlying container instead
	/// @tparam Bits Number of bits in the set
	/// @tparam UnderlyingType Underlying integral type, defaults to smallest integer that represents Bits, if Bits is
	/// greater than maximum single value then stores an array of UnderlyingTpye
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
		using size_type = typename base::size_type;

		using base::base;

		/// @brief Construct from underlying_type, only enabled if Bits fits into one UnderlyingType
		constexpr bitset( const underlying_type value )
			requires( num_values == 1 )
			: base( std::array{ value } )
		{
		}

		/// @brief Construct from a string like type of unset_char and set_char
		template <typename StringLike, typename CharT = typename StringLike::value_type>
		constexpr explicit bitset( const StringLike& str,
								   const CharT unset_char = CharT( '0' ),
								   const CharT set_char = CharT( '1' ) )
		{
			using view = std::basic_string_view<CharT, typename StringLike::traits_type>;
			base::init_from_string( view( str ), unset_char, set_char );
		}

		/// @brief Convert to an unsigned long, only enabled if Bits fits into one unsigned long or smaller
		/// @return Bitset underlying value as unsigned long
		[[nodiscard]] constexpr unsigned long to_ulong() const noexcept
			requires( num_values == 1 && sizeof( underlying_type ) <= sizeof( unsigned long ) )
		{
			return static_cast<unsigned long>( base::underlying().front() );
		}

		/// @brief Convert to an unsigned long long, only enabled if Bits fits into one unsigned long long or smaller
		/// @return Bitset underlying value as unsigned long long
		[[nodiscard]] constexpr unsigned long long to_ullong() const noexcept
			requires( num_values == 1 && sizeof( underlying_type ) <= sizeof( unsigned long long ) )
		{
			return static_cast<unsigned long long>( base::underlying().front() );
		}

		[[nodiscard]] constexpr bool operator==( const bitset& other ) const noexcept = default;

	private:
		static constexpr bool last_needs_mask = Bits % base::bits_per_value != 0;
		static constexpr UnderlyingType last_mask = ( base::one << ( Bits % base::bits_per_value ) ) - 1;

		[[nodiscard]] static constexpr size_type derived_size() noexcept
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
	struct hash<mclo::bitset<Bits, UnderlyingType>> : mclo::hash<mclo::bitset<Bits, UnderlyingType>>
	{
	};
}
