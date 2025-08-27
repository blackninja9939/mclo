#pragma once

#include "mclo/container/span.hpp"
#include "mclo/preprocessor/platform.hpp"

#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>
#undef XXH_STATIC_LINKING_ONLY

namespace mclo
{
	// Prefer for hashing a small amount of data, such as the default hasher, as the stack size is not too big
	// For hashing directly a single object/byte range use XXH64 directly
	class xxhash_64
	{
	public:
		explicit xxhash_64( const XXH64_hash_t seed = 0 ) noexcept;

		void write( const mclo::span<const std::byte> data ) noexcept;
		[[nodiscard]] std::size_t finish() const noexcept;

	private:
		XXH64_state_t m_state;
	};

	// Prefer for hashing a large amount of data, internally allocates the hash state due to its size
	// For hashing directly a single object/byte range use XXH3_64bits directly
	class xxhash_3
	{
	public:
		explicit xxhash_3( const XXH64_hash_t seed = 0 );

		void write( const mclo::span<const std::byte> data ) noexcept;
		[[nodiscard]] std::size_t finish() const noexcept;

	private:
		struct deleter
		{
			MCLO_STATIC_CALL_OPERATOR void operator()( XXH3_state_t* data ) MCLO_CONST_CALL_OPERATOR noexcept;
		};

		std::unique_ptr<XXH3_state_t, deleter> m_state;
	};
}
