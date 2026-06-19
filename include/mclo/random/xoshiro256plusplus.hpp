#pragma once

#include <cinttypes>
#include <limits>

namespace mclo
{
	/// @brief A fast 64-bit pseudo-random number generator based on the xoshiro256++ algorithm.
	/// @details xoshiro256++ maintains 256 bits of state and combines bit shifts, rotations and XORs to produce
	/// high-quality 64-bit output with a period of 2^256 - 1. It satisfies the standard @c UniformRandomBitGenerator
	/// requirements and so can be used with the @c <random> distribution utilities. It is well suited as a general
	/// purpose generator where speed and statistical quality matter but cryptographic security does not.
	/// @note The state is seeded by expanding the given 64-bit seed with @c splitmix64 to ensure good distribution.
	/// @see splitmix64
	/// @warning Not cryptographically secure; do not use for security-sensitive purposes.
	class xoshiro256plusplus
	{
		static constexpr std::uint64_t default_seed = 1;

	public:
		/// @brief The unsigned integer type produced by the generator.
		using result_type = std::uint64_t;

		/// @brief The smallest value the generator can produce.
		/// @return The minimum value of @c result_type.
		static constexpr result_type min() noexcept
		{
			return std::numeric_limits<result_type>::min();
		}

		/// @brief The largest value the generator can produce.
		/// @return The maximum value of @c result_type.
		static constexpr result_type max() noexcept
		{
			return std::numeric_limits<result_type>::max();
		}

		/// @brief Constructs the generator with the given seed.
		/// @param seed The seed used to initialise the 256-bit state.
		xoshiro256plusplus( const std::uint64_t seed = default_seed ) noexcept;

		/// @brief Reseeds the generator, resetting its state.
		/// @param seed The seed used to re-initialise the 256-bit state.
		void seed( std::uint64_t seed ) noexcept;

		/// @brief Advances the state and returns the next pseudo-random value.
		/// @return The next 64-bit pseudo-random value in the sequence.
		result_type operator()() noexcept;

		/// @brief Advances the generator past the next @p count values.
		/// @param count The number of values to skip.
		void discard( unsigned long long count ) noexcept;

		/// @brief Advances the state by 2^128 calls to @c operator().
		/// @details Equivalent to 2^128 calls to @c operator(); it can be used to generate 2^128 non-overlapping
		/// subsequences for parallel computations.
		void jump() noexcept;

		/// @brief Advances the state by 2^192 calls to @c operator().
		/// @details Equivalent to 2^192 calls to @c operator(); it can be used to generate 2^64 starting points,
		/// from each of which @c jump() will generate 2^64 non-overlapping subsequences for parallel distributed
		/// computations.
		void long_jump() noexcept;

		/// @brief Compares two generators for equality.
		/// @param other The generator to compare against.
		/// @return @c true if both generators have identical state and will produce the same sequence.
		bool operator==( const xoshiro256plusplus& other ) const noexcept = default;

	private:
		std::uint64_t state[ 4 ];
	};
}
