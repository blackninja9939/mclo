#pragma once

#include "mclo/hash/hash.hpp"
#include "mclo/numeric/pow2.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <compare>
#include <limits>

namespace mclo
{
	/// @brief Slot map handle of a given bit size for a given type
	/// @details Base type is not templated on T to reduce template instantiations, use slot_map_handle for APIs
	/// @tparam TotalBits The total number of bits for the handle type to use
	/// @tparam GenerationBits The number of bits in the handle type to use for generation checking
	template <std::size_t TotalBits, std::size_t GenerationBits>
	struct slot_map_handle_base
	{
	public:
		static_assert( GenerationBits > 0, "Must have some bits for the generation" );

		using representation_type = uint_least_t<TotalBits>;

	private:
		static constexpr std::size_t index_bits = TotalBits - GenerationBits;
		static_assert( index_bits > 1, "At least two bits are needed for index, one is used for null state" );

		static constexpr auto generation_bits_mask = std::numeric_limits<representation_type>::max() << index_bits;
		static constexpr representation_type index_bits_mask = ~generation_bits_mask;

	public:
		// 2^Bits - 1 = Max value we can fit in the bits, -1 to account for 0
		// max_index is -2 because we use the actual max for a null state
		static constexpr auto max_index = mclo::pow2<representation_type>( index_bits ) - 2;
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

		/// @brief Check if the handle is null
		/// @warning This does NOT guarantee the handle refers to a valid object, all it does is a cheap check if the
		/// handle has the potential to refer to an object
		/// @details Since we always have at least a bit used for generation we know our index value is never greater
		/// than the max_index but that value also still fits within the representation type, so we can use it as the
		/// invalid sentinel value
		[[nodiscard]] bool is_null() const noexcept
		{
			return index == max_index + 1;
		}

		/// @brief Index of the handle into its slot map
		representation_type index : index_bits = max_index + 1;

		/// @brief Generation of the handle for validating if it refers to a valid object
		representation_type generation : GenerationBits = max_generation;
	};

	/// @brief Slot map handle of a given bit size for a given type
	/// @details By being a template on T we make the handle type safe so you can not look up handles of a mismatching
	/// type as it will be a compile time error instead of giving you garbage returns
	/// @tparam T Type this handle refers to
	/// @tparam TotalBits The total number of bits for the handle type to use
	/// @tparam GenerationBits The number of bits in the handle type to use for generation checking
	template <typename T, std::size_t TotalBits, std::size_t GenerationBits>
	struct slot_map_handle : slot_map_handle_base<TotalBits, GenerationBits>
	{
		using value_type = T;

		[[nodiscard]] constexpr auto operator<=>( const slot_map_handle& other ) const noexcept
		{
			return this->get_combined() <=> other.get_combined();
		}
		[[nodiscard]] constexpr bool operator==( const slot_map_handle& other ) const noexcept
		{
			return this->get_combined() == other.get_combined();
		}
	};

	template <typename T, std::size_t TotalBits, std::size_t GenerationBits>
	constexpr bool enable_bitwise_hash<slot_map_handle<T, TotalBits, GenerationBits>> = true;
}

namespace std
{
	template <typename T, std::size_t TotalBits, std::size_t GenerationBits>
	struct hash<mclo::slot_map_handle<T, TotalBits, GenerationBits>>
		: mclo::hash<mclo::slot_map_handle<T, TotalBits, GenerationBits>>
	{
	};
}
