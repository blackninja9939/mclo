#pragma once

#include <cinttypes>
#include <limits>

namespace mclo
{
	class splitmix64
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

		splitmix64( const std::uint64_t seed = default_seed ) noexcept;

		void seed( const std::uint64_t seed = default_seed ) noexcept;

		result_type operator()() noexcept;

		void discard( unsigned long long count ) noexcept;

		bool operator==( const splitmix64& other ) const noexcept = default;

	private:
		std::uint64_t state = default_seed;
	};
}
