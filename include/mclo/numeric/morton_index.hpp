#pragma once

#include "mclo/hash/hash.hpp"
#include "mclo/numeric/bit.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

namespace mclo
{
	/// @brief A Morton (Z-order curve) index that interleaves two coordinates into a single value.
	/// @details Interleaving the bits of the x and y coordinates produces an index whose ordering preserves
	/// spatial locality, making it useful for spatial data structures and cache-friendly traversal of 2D grids.
	/// @tparam T The unsigned integer type used to store the interleaved index. Its bit width determines the
	/// available range of each coordinate.
	template <std::unsigned_integral T>
	class morton_index
	{
	public:
		/// @brief The unsigned integer type storing the interleaved index.
		using value_type = T;

		/// @brief The unsigned integer type of a single coordinate, half the bit width of @c value_type.
		using coord_type = mclo::uint_least_t<sizeof( value_type ) * CHAR_BIT / 2>;
		static_assert( !std::is_same_v<coord_type, void>, "No suitable coord_type for this T" );

		/// @brief Constructs a Morton index with a zero value.
		constexpr morton_index() noexcept = default;

		/// @brief Constructs a Morton index directly from an already-interleaved value.
		/// @param value The pre-encoded interleaved index value.
		constexpr explicit morton_index( const value_type value ) noexcept
			: value( value )
		{
		}

		/// @brief Constructs a Morton index by interleaving the given coordinates.
		/// @param x The x coordinate.
		/// @param y The y coordinate.
		constexpr morton_index( const coord_type x, const coord_type y ) noexcept
			: value( encode( x, y ) )
		{
		}

		/// @brief Extracts the original coordinates from the interleaved index.
		/// @return A pair of the x and y coordinates.
		[[nodiscard]] constexpr std::pair<coord_type, coord_type> decode() const noexcept
		{
			const coord_type x = static_cast<coord_type>( mclo::bit_compress( value, x_mask ) );
			const coord_type y = static_cast<coord_type>( mclo::bit_compress( value, y_mask ) );
			return { x, y };
		}

		/// @brief Compares two Morton indices by their interleaved value, giving Z-order ordering.
		[[nodiscard]] constexpr auto operator<=>( const morton_index& rhs ) const noexcept = default;

		/// @brief Appends this Morton index value to a hasher, hashing by its underlying representation.
		/// @param hasher The hasher to append to.
		/// @param value The Morton index value to hash.
		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const morton_index& value ) noexcept
		{
			hash_append( hasher, value.value );
		}

		/// @brief The interleaved index value.
		value_type value;

	private:
		static constexpr value_type x_mask = static_cast<value_type>( 0xAAAAAAAAAAAAAAAAull );
		static constexpr value_type y_mask = static_cast<value_type>( 0x5555555555555555ull );

		[[nodiscard]] static constexpr value_type encode( const coord_type x, const coord_type y ) noexcept
		{
			const value_type masked_x = mclo::bit_expand( x, static_cast<coord_type>( x_mask ) );
			const value_type masked_y = mclo::bit_expand( y, static_cast<coord_type>( y_mask ) );
			return masked_x | masked_y;
		}
	};

	/// @brief A Morton index backed by a 64-bit value, giving 32-bit coordinates.
	using morton_index64 = morton_index<std::uint64_t>;

	/// @brief A Morton index backed by a 32-bit value, giving 16-bit coordinates.
	using morton_index32 = morton_index<std::uint32_t>;

	/// @brief A Morton index backed by a 16-bit value, giving 8-bit coordinates.
	using morton_index16 = morton_index<std::uint16_t>;
}

template <std::unsigned_integral T>
struct std::hash<mclo::morton_index<T>> : mclo::hash<mclo::morton_index<T>>
{
};
