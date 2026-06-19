#pragma once

#include <cinttypes>
#include <limits>

namespace mclo
{
	/// @brief A fast 64-bit pseudo-random number generator based on the SplitMix64 algorithm.
	/// @details SplitMix64 advances a single 64-bit state by a fixed additive constant and applies a finalising
	/// mixing function to each output, making it extremely fast with minimal state. It satisfies the standard
	/// @c UniformRandomBitGenerator requirements and so can be used with the @c <random> distribution utilities.
	/// @note Its primary use is seeding other generators with larger state (such as @c xoshiro256plusplus) from a
	/// single 64-bit seed, since it produces well-distributed output even from poor seeds.
	class splitmix64
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
		/// @param seed The initial state of the generator.
		splitmix64( const std::uint64_t seed = default_seed ) noexcept;

		/// @brief Reseeds the generator, resetting its state.
		/// @param seed The new state of the generator.
		void seed( const std::uint64_t seed = default_seed ) noexcept;

		/// @brief Advances the state and returns the next pseudo-random value.
		/// @return The next 64-bit pseudo-random value in the sequence.
		result_type operator()() noexcept;

		/// @brief Advances the generator past the next @p count values.
		/// @param count The number of values to skip.
		void discard( unsigned long long count ) noexcept;

		/// @brief Compares two generators for equality.
		/// @param other The generator to compare against.
		/// @return @c true if both generators have identical state and will produce the same sequence.
		bool operator==( const splitmix64& other ) const noexcept = default;

	private:
		std::uint64_t state = default_seed;
	};
}
