#pragma once

#include <concepts>
#include <cstdint>

namespace mclo
{
	/// @brief Diffuses every input bit across all output bits.
	/// @details Applies an xor-shift-multiply avalanche so that flipping any single input bit changes roughly half
	/// the output bits, while remaining a bijection so distinct inputs always yield distinct outputs. Sequential or
	/// low-entropy inputs such as counters or grid coordinates are therefore turned into well-distributed,
	/// decorrelated results.
	/// @note Also serves as the finalizer for this library's @c splitmix64 generator.
	/// @param value The value to diffuse.
	/// @return The fully diffused value.
	[[nodiscard]] constexpr std::uint64_t avalanche_bits( std::uint64_t value ) noexcept
	{
		value = ( value ^ ( value >> 30 ) ) * UINT64_C( 0xbf58476d1ce4e5b9 );
		value = ( value ^ ( value >> 27 ) ) * UINT64_C( 0x94d049bb133111eb );
		return value ^ ( value >> 31 );
	}

	namespace detail
	{
		/// @brief Folds a single position into an accumulating seed with full avalanche between steps.
		/// @details Each position is independently mixed before being folded into the running seed, so the result is
		/// free of the linear collisions a plain weighted sum of positions would have.
		/// @param seed The running seed accumulator.
		/// @param position The next position to fold in.
		/// @return The updated seed.
		[[nodiscard]] constexpr std::uint64_t mix_seed_into( const std::uint64_t seed,
															 const std::uint64_t position ) noexcept
		{
			// Multiplied by a golden-ratio derived odd constant to advance the running seed between positions.
			return avalanche_bits( seed * UINT64_C( 0x9e3779b97f4a7c15 ) + avalanche_bits( position ) );
		}
	}

	/// @brief Mixes any number of positions into a single well-distributed seed.
	/// @details Folds each position into a running accumulator via @c avalanche_bits, so the result is sensitive to
	/// order (permutations of the positions diverge), free of the collisions a plain weighted sum would suffer, and
	/// decorrelated for adjacent positions. Intended for deriving a deterministic per-object or per-cell seed from an
	/// identifier or grid coordinate before passing it to a generator such as @c xoshiro256plusplus.
	/// @tparam Positions The integral position types.
	/// @param positions The positions to mix together.
	/// @return The combined seed.
	template <std::integral... Positions>
		requires( sizeof...( Positions ) >= 1 )
	[[nodiscard]] constexpr std::uint64_t mix_seed( const Positions... positions ) noexcept
	{
		// Non-zero starting basis so that mixing all-zero positions still yields a well-distributed seed.
		std::uint64_t seed = UINT64_C( 0x243f6a8885a308d3 );
		( ( seed = detail::mix_seed_into( seed, static_cast<std::uint64_t>( positions ) ) ), ... );
		return seed;
	}
}
