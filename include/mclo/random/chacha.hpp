#pragma once

#include <array>
#include <cinttypes>
#include <limits>

namespace mclo
{
	/// @brief A cryptographically strong pseudo-random number generator based on the ChaCha stream cipher.
	/// @details This generator produces a keystream using the ChaCha algorithm, seeded with a 256-bit key and a
	/// 96-bit nonce. The number of rounds is configurable via @p Rounds, trading security margin against speed;
	/// the canonical variants are exposed as the @c chacha8, @c chacha12, and @c chacha20 aliases. The keystream
	/// is exposed as a sequence of 64-bit values, satisfying the standard @c UniformRandomBitGenerator requirements
	/// so it can be used with the @c <random> distribution utilities. Unlike @c xoshiro256plusplus and
	/// @c splitmix64, ChaCha is suitable for security-sensitive purposes.
	/// @tparam Rounds The number of rounds to perform per block; must be a non-zero even number.
	/// @note The block counter can be set directly to seek within the keystream in constant time.
	/// @see set_counter
	template <std::size_t Rounds>
	class chacha
	{
		static_assert( Rounds != 0, "Number of rounds must be non-zero" );
		static_assert( Rounds % 2 == 0, "Number of rounds must be even" );

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
		chacha( const std::array<std::uint8_t, 32>& seed, const std::array<std::uint8_t, 12>& nonce ) noexcept;

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
		[[nodiscard]] bool operator==( const chacha& other ) const noexcept;

	private:
		void generate_block() noexcept;

		static constexpr std::uint8_t keystream_max = 8;

		std::array<std::uint32_t, 16> state;
		std::array<result_type, keystream_max> keystream{};
		std::uint8_t keystream_index = 0;
	};

	/// @brief ChaCha generator using 8 rounds: the fastest variant, with the smallest security margin.
	using chacha8 = chacha<8>;

	/// @brief ChaCha generator using 12 rounds: a middle ground between speed and security margin.
	using chacha12 = chacha<12>;

	/// @brief ChaCha generator using 20 rounds: the most conservative variant, recommended for general use.
	using chacha20 = chacha<20>;

	extern template class chacha<8>;
	extern template class chacha<12>;
	extern template class chacha<20>;
}
