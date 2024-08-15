#pragma once

#include "mclo/math.hpp"
#include "mclo/standard_integer_type.hpp"

#include <limits>

namespace mclo
{
	/// @brief Slot map handle of a given bit size for a given type
	/// @details By being a template on T we make the handle type safe so you can not look up handles of a mismatching
	/// type as it will be a compile time error instead of giving you garbage returns
	/// @tparam T Type this handle refers to
	/// @tparam TotalBits The total number of bits for the handle type to use
	/// @tparam GenerationBits The number of bits in the handle type to use for generation checking
	template <typename T, std::size_t TotalBits, std::size_t GenerationBits>
	struct slot_map_handle
	{
	public:
		static_assert( GenerationBits > 0, "Must have some bits for the generation" );
		static_assert( TotalBits > GenerationBits, "Must have some bits for the index" );

		using representation_type = uint_least_t<TotalBits>;
		using value_type = T;

	private:
		static constexpr std::size_t index_bits = TotalBits - GenerationBits;
		static_assert( index_bits > 0 );

		static constexpr auto generation_bits_mask = std::numeric_limits<representation_type>::max() << index_bits;
		static constexpr representation_type index_bits_mask = ~generation_bits_mask;

	public:
		// 2^Bits - 1 = Max value we can fit in the bits, -1 to account for 0
		static constexpr auto max_index = mclo::pow2<representation_type>( index_bits ) - 1;
		static constexpr auto max_generation = mclo::pow2<representation_type>( GenerationBits ) - 1;

		/// @brief Get the handle as a single combined value with the generation stored in the upper bits and index in
		/// the lower
		/// @return Combined handle value
		[[nodiscard]] constexpr representation_type get_combined() const noexcept
		{
			return ( generation << index_bits ) | index;
		}

		/// @brief Set the handles index and generation from a combined value, generation must be in the upper bits and
		/// index in the lower
		/// @param Value The combined value
		constexpr void set_combined( const representation_type value ) noexcept
		{
			generation = value >> index_bits;
			index = value & index_bits_mask;
		}

		[[nodiscard]] constexpr auto operator<=>( const slot_map_handle& other ) const noexcept = default;

		/// @brief Index of the handle into its slot map
		representation_type index : index_bits;

		/// @brief Generation of the handle for validating if it refers to a valid object
		representation_type generation : GenerationBits;
	};
}
