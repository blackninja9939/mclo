#pragma once

#include "mclo/container/span.hpp"
#include "mclo/platform/cpp_feature_compat.hpp"

#include <memory>

#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>
#undef XXH_STATIC_LINKING_ONLY

namespace mclo
{
	/// @brief A hasher implementing the 64-bit XXH64 algorithm.
	/// @details Satisfies the @ref hasher concept. The hash state lives on the stack, making this preferable for
	/// hashing small amounts of data such as the @ref default_hasher use case.
	class xxhash_64
	{
	public:
		/// @brief Constructs the hasher with an optional seed.
		/// @param seed The seed value mixed into the hash.
		explicit xxhash_64( const XXH64_hash_t seed = 0 ) noexcept;

		/// @brief Mixes @p data into the running hash.
		/// @param data The bytes to hash.
		void write( const mclo::span<const std::byte> data ) noexcept;

		/// @brief Finalises and returns the hash value.
		/// @return The computed hash.
		[[nodiscard]] std::size_t finish() const noexcept;

	private:
		XXH64_state_t m_state;
	};

	/// @brief A hasher implementing the XXH3 64-bit algorithm.
	/// @details Satisfies the @ref hasher concept. The hash state is large and so is heap-allocated; prefer this for
	/// hashing large amounts of data.
	class xxhash_3
	{
	public:
		/// @brief Constructs the hasher with an optional seed, allocating the hash state.
		/// @param seed The seed value mixed into the hash.
		explicit xxhash_3( const XXH64_hash_t seed = 0 );

		/// @brief Mixes @p data into the running hash.
		/// @param data The bytes to hash.
		void write( const mclo::span<const std::byte> data ) noexcept;

		/// @brief Finalises and returns the hash value.
		/// @return The computed hash.
		[[nodiscard]] std::size_t finish() const noexcept;

	private:
		struct deleter
		{
			MCLO_STATIC_CALL_OPERATOR void operator()( XXH3_state_t* data ) MCLO_CONST_CALL_OPERATOR noexcept;
		};

		std::unique_ptr<XXH3_state_t, deleter> m_state;
	};
}
