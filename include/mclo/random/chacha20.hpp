#pragma once

#include "mclo/platform/warnings.hpp"

#include <array>
#include <cinttypes>
#include <limits>

namespace mclo
{
	MCLO_MSVC_PUSH_AND_DISABLE_WARNINGS( 4324 ) // structure was padded due to alignment specifier

	/// @brief A cryptographically strong pseudo-random number generator based on the ChaCha20 stream cipher.
	/// @details This generator produces a keystream using the ChaCha20 algorithm, seeded with a 256-bit key and a
	/// 96-bit nonce. The keystream is exposed as a sequence of 64-bit values, satisfying the standard
	/// @c UniformRandomBitGenerator requirements so it can be used with the @c <random> distribution utilities.
	/// Unlike @c xoshiro256plusplus and @c splitmix64, ChaCha20 is suitable for security-sensitive purposes.
	/// @note The block counter can be set directly to seek within the keystream in constant time.
	/// @see set_counter
	class alignas( 16 ) chacha20
	{
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

		/// @brief Constructs the generator with the given key and nonce.
		/// @param seed The 256-bit key used to initialise the cipher.
		/// @param nonce The 96-bit nonce used to initialise the cipher.
		chacha20( const std::array<std::uint8_t, 32>& seed, const std::array<std::uint8_t, 12>& nonce ) noexcept;

		/// @brief Reseeds the generator with a new key and nonce, resetting its state.
		/// @param seed The new 256-bit key.
		/// @param nonce The new 96-bit nonce.
		void seed( const std::array<std::uint8_t, 32>& seed, const std::array<std::uint8_t, 12>& nonce ) noexcept;

		/// @brief Advances the keystream and returns the next pseudo-random value.
		/// @return The next 64-bit value from the keystream.
		result_type operator()() noexcept;

		/// @brief Advances the generator past the next @p count values.
		/// @param count The number of values to skip.
		void discard( unsigned long long count ) noexcept;

		/// @brief Jumps to the start of the given block in the keystream in constant time.
		/// @param block The keystream block index to seek to.
		void set_counter( std::uint32_t block ) noexcept;

		/// @brief Gets the current keystream block counter.
		/// @return The index of the current keystream block.
		[[nodiscard]] std::uint32_t get_counter() const noexcept
		{
			return state[ 12 ];
		}

		/// @brief Compares two generators for equality.
		/// @param other The generator to compare against.
		/// @return @c true if both generators have identical state and will produce the same sequence.
		[[nodiscard]] bool operator==( const chacha20& other ) const noexcept;

	private:
		void generate_block() noexcept;

		std::array<std::uint32_t, 16> state;
		std::array<result_type, 8> keystream{};
		std::uint8_t keystream_index = 0;
	};

	MCLO_MSVC_POP_WARNINGS
}
