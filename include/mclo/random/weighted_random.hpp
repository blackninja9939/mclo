#pragma once

#include "mclo/debug/assert.hpp"

#include <concepts>
#include <functional>
#include <iterator>
#include <random>
#include <ranges>

namespace mclo
{
	/// @brief Selects a random element index from a range, weighted by a projected weight per element.
	/// @details Computes each element's weight by invoking @p proj on it, sums them, draws a uniformly distributed
	/// value in @c [0, total) and walks the range to find the corresponding element. The projection supports
	/// computing weights on demand for lightweight calculations, or reading a precomputed member via a
	/// pointer-to-member. A default projection of @c std::identity treats the elements themselves as the weights, so
	/// a precomputed side-by-side container of weights can be passed directly and used to index the parallel range.
	/// @tparam Engine The random bit generator type, satisfying @c std::uniform_random_bit_generator.
	/// @tparam Rng The forward range type containing the candidate elements.
	/// @tparam Proj A projection invocable on each element yielding an integral weight.
	/// @param engine The random bit generator to sample from.
	/// @param rng The range of candidate elements.
	/// @param proj The projection used to obtain each element's weight.
	/// @return The index of the selected element.
	/// @pre @p rng must not be empty, all weights must be non-negative, and the total weight must be positive.
	template <std::uniform_random_bit_generator Engine,
			  std::ranges::forward_range Rng,
			  std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Proj = std::identity>
		requires std::integral<std::remove_cvref_t<std::indirect_result_t<Proj&, std::ranges::iterator_t<Rng>>>>
	[[nodiscard]] std::ranges::range_difference_t<Rng> weighted_index( Engine& engine, Rng&& rng, Proj proj = {} )
	{
		using weight_type = std::remove_cvref_t<std::indirect_result_t<Proj&, std::ranges::iterator_t<Rng>>>;

		const auto begin = std::ranges::begin( rng );
		const auto end = std::ranges::end( rng );
		MCLO_DEBUG_ASSERT( begin != end, "Range must not be empty" );

		weight_type total = 0;
		for ( auto it = begin; it != end; ++it )
		{
			const weight_type weight = std::invoke( proj, *it );
			MCLO_DEBUG_ASSERT( weight >= 0, "Weights must be non-negative" );
			total += weight;
		}
		MCLO_DEBUG_ASSERT( total > 0, "Total weight must be positive" );

		weight_type roll = std::uniform_int_distribution<weight_type>( 0, total - 1 )( engine );
		std::ranges::range_difference_t<Rng> index = 0;
		for ( auto it = begin; it != end; ++it, ++index )
		{
			const weight_type weight = std::invoke( proj, *it );
			if ( roll < weight )
			{
				return index;
			}
			roll -= weight;
		}

		MCLO_UNREACHABLE( "Total weight is positive so the roll must land on an element" );
	}
}
