#include "mclo/random/xoshiro256plusplus.hpp"

#include "mclo/random/splitmix64.hpp"

#include <bit>

mclo::xoshiro256plusplus::xoshiro256plusplus( const std::uint64_t seed ) noexcept
{
	this->seed( seed );
}

void mclo::xoshiro256plusplus::seed( const std::uint64_t seed ) noexcept
{
	splitmix64 splitmix( seed );
	for ( std::uint64_t& value : state )
	{
		value = splitmix();
	}
}

mclo::xoshiro256plusplus::result_type mclo::xoshiro256plusplus::operator()() noexcept
{
	const result_type result = std::rotl( state[ 0 ] + state[ 3 ], 23 ) + state[ 0 ];

	const result_type t = state[ 1 ] << 17;

	state[ 2 ] ^= state[ 0 ];
	state[ 3 ] ^= state[ 1 ];
	state[ 1 ] ^= state[ 2 ];
	state[ 0 ] ^= state[ 3 ];

	state[ 2 ] ^= t;

	state[ 3 ] = std::rotl( state[ 3 ], 45 );

	return result;
}

void mclo::xoshiro256plusplus::discard( unsigned long long count ) noexcept
{
	while ( count-- )
	{
		( void )operator()();
	}
}

void mclo::xoshiro256plusplus::jump() noexcept
{
	static constexpr std::uint64_t JUMP_TABLE[] = {
		0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };

	std::uint64_t s0 = 0;
	std::uint64_t s1 = 0;
	std::uint64_t s2 = 0;
	std::uint64_t s3 = 0;

	for ( const std::uint64_t jump : JUMP_TABLE )
	{
		for ( int b = 0; b < 64; b++ )
		{
			if ( jump & UINT64_C( 1 ) << b )
			{
				s0 ^= state[ 0 ];
				s1 ^= state[ 1 ];
				s2 ^= state[ 2 ];
				s3 ^= state[ 3 ];
			}
			operator()();
		}
	}

	state[ 0 ] = s0;
	state[ 1 ] = s1;
	state[ 2 ] = s2;
	state[ 3 ] = s3;
}

void mclo::xoshiro256plusplus::long_jump() noexcept
{
	static constexpr std::uint64_t LONG_JUMP_TABLE[] = {
		0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };

	std::uint64_t s0 = 0;
	std::uint64_t s1 = 0;
	std::uint64_t s2 = 0;
	std::uint64_t s3 = 0;

	for ( const std::uint64_t jump : LONG_JUMP_TABLE )
	{
		for ( int b = 0; b < 64; b++ )
		{
			if ( jump & UINT64_C( 1 ) << b )
			{
				s0 ^= state[ 0 ];
				s1 ^= state[ 1 ];
				s2 ^= state[ 2 ];
				s3 ^= state[ 3 ];
			}
			operator()();
		}
	}

	state[ 0 ] = s0;
	state[ 1 ] = s1;
	state[ 2 ] = s2;
	state[ 3 ] = s3;
}
