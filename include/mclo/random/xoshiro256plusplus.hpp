#pragma once

#include <cinttypes>
#include <limits>

namespace mclo
{
	class xoshiro256plusplus
	{
		static constexpr std::uint64_t default_seed = 1;

	public:
		using result_type = std::uint64_t;

		static constexpr result_type min() noexcept
		{
			return std::numeric_limits<result_type>::min();
		}
		static constexpr result_type max() noexcept
		{
			return std::numeric_limits<result_type>::max();
		}

		xoshiro256plusplus( const std::uint64_t seed = default_seed ) noexcept;
		void seed( std::uint64_t seed ) noexcept;

		result_type operator()() noexcept;

		void discard( unsigned long long count ) noexcept;

		/* This is the jump function for the generator. It is equivalent
		   to 2^128 calls to operator(); it can be used to generate 2^128
		   non-overlapping subsequences for parallel computations. */
		void jump() noexcept;

		/* This is the long-jump function for the generator. It is equivalent to
		   2^192 calls to operator(); it can be used to generate 2^64 starting points,
		   from each of which jump() will generate 2^64 non-overlapping
		   subsequences for parallel distributed computations. */
		void long_jump() noexcept;

		bool operator==( const xoshiro256plusplus& other ) const noexcept = default;

	private:
		std::uint64_t state[ 4 ];
	};
}
