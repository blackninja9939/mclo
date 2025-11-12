#pragma once

#include "mclo/hash/hash.hpp"
#include "mclo/numeric/bit.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

namespace mclo
{
	template <std::unsigned_integral T>
	class morton_index
	{
	public:
		using value_type = T;
		using coord_type = mclo::uint_least_t<sizeof( value_type ) * CHAR_BIT / 2>;
		static_assert( !std::is_same_v<coord_type, void>, "No suitable coord_type for this T" );

		constexpr morton_index() noexcept = default;

		constexpr explicit morton_index( const value_type value ) noexcept
			: value( value )
		{
		}

		constexpr morton_index( const coord_type x, const coord_type y ) noexcept
			: value( encode( x, y ) )
		{
		}

		[[nodiscard]] constexpr std::pair<coord_type, coord_type> decode() const noexcept
		{
			const coord_type x = static_cast<coord_type>( mclo::bit_compress( value, x_mask ) );
			const coord_type y = static_cast<coord_type>( mclo::bit_compress( value, y_mask ) );
			return { x, y };
		}

		[[nodiscard]] constexpr auto operator<=>( const morton_index& rhs ) const noexcept = default;

		template <hasher Hasher>
		friend void hash_append( Hasher& hasher, const morton_index& value ) noexcept
		{
			hash_append( hasher, value.value );
		}

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

	using morton_index64 = morton_index<std::uint64_t>;
	using morton_index32 = morton_index<std::uint32_t>;
	using morton_index16 = morton_index<std::uint16_t>;
}

template <std::unsigned_integral T>
struct std::hash<mclo::morton_index<T>> : mclo::hash<mclo::morton_index<T>>
{
};
