#include "mclo/random/splitmix64.hpp"

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
	result_type z = state;
	z = ( z ^ ( z >> 30 ) ) * UINT64_C( 0xbf58476d1ce4e5b9 );
	z = ( z ^ ( z >> 27 ) ) * UINT64_C( 0x94d049bb133111eb );
	return z ^ ( z >> 31 );
}

void mclo::splitmix64::discard( unsigned long long count ) noexcept
{
	state += count * offset;
}
