#include "mclo/random/splitmix64.hpp"

#include "mclo/random/seed_mixing.hpp"

namespace
{
	constexpr std::uint64_t offset = UINT64_C( 0x9e3779b97f4a7c15 );
}

mclo::splitmix64::splitmix64( const std::uint64_t seed ) noexcept
	: state( seed )
{
}

void mclo::splitmix64::seed( const std::uint64_t seed ) noexcept
{
	state = seed;
}

mclo::splitmix64::result_type mclo::splitmix64::operator()() noexcept
{
	state += offset;
	return mclo::avalanche_bits( state );
}

void mclo::splitmix64::discard( unsigned long long count ) noexcept
{
	state += count * offset;
}
