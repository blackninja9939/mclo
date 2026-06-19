#include "mclo/random/chacha.hpp"

#include "mclo/numeric/bit.hpp"

#include <algorithm>
#include <cstring>

namespace
{
	[[nodiscard]] std::uint32_t load_le32( const std::uint8_t* const bytes ) noexcept
	{
		std::uint32_t value;
		std::memcpy( &value, bytes, sizeof( value ) );
		if constexpr ( std::endian::native == std::endian::big )
		{
			value = mclo::byteswap( value );
		}
		return value;
	}

	void quarter_round( std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d ) noexcept
	{
		a += b;
		d ^= a;
		d = std::rotl( d, 16 );
		c += d;
		b ^= c;
		b = std::rotl( b, 12 );
		a += b;
		d ^= a;
		d = std::rotl( d, 8 );
		c += d;
		b ^= c;
		b = std::rotl( b, 7 );
	}
}

template <std::size_t Rounds>
mclo::chacha<Rounds>::chacha( const std::array<std::uint8_t, 32>& seed,
							  const std::array<std::uint8_t, 12>& nonce ) noexcept
	: state{ 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 } // "expand 32-byte k" constants
{
	this->seed( seed, nonce );
}

template <std::size_t Rounds>
void mclo::chacha<Rounds>::seed( const std::array<std::uint8_t, 32>& seed,
								 const std::array<std::uint8_t, 12>& nonce ) noexcept
{
	// Key
	for ( std::size_t i = 0; i < 8; ++i )
	{
		state[ 4 + i ] = load_le32( seed.data() + 4 * i );
	}

	// Counter
	state[ 12 ] = 0;

	// Nonce
	for ( std::size_t i = 0; i < 3; ++i )
	{
		state[ 13 + i ] = load_le32( nonce.data() + 4 * i );
	}

	keystream_index = keystream_max; // Force generation of new keystream on first use
}

template <std::size_t Rounds>
typename mclo::chacha<Rounds>::result_type mclo::chacha<Rounds>::operator()() noexcept
{
	if ( keystream_index >= keystream_max )
	{
		generate_block();
	}
	return keystream[ keystream_index++ ];
}

template <std::size_t Rounds>
void mclo::chacha<Rounds>::discard( unsigned long long count ) noexcept
{
	// Consume whatever is still buffered in the current block first.
	const unsigned long long buffered = keystream_max - keystream_index;
	if ( count < buffered )
	{
		keystream_index += static_cast<std::uint8_t>( count );
		return;
	}

	// Now positioned on a block boundary, so jump whole blocks via the counter. The uint32_t truncation matches the
	// wraparound of that many individual counter increments.
	count -= buffered;
	state[ 12 ] += static_cast<std::uint32_t>( count / keystream_max );

	const std::uint8_t remainder = static_cast<std::uint8_t>( count % keystream_max );
	if ( remainder == 0 )
	{
		keystream_index = keystream_max; // Force generation of the new block on next use
	}
	else
	{
		generate_block();
		keystream_index = remainder;
	}
}

template <std::size_t Rounds>
void mclo::chacha<Rounds>::set_counter( const std::uint32_t block ) noexcept
{
	state[ 12 ] = block;
	keystream_index = keystream_max; // Force generation at the new position on next use
}

template <std::size_t Rounds>
void mclo::chacha<Rounds>::generate_block() noexcept
{
	std::array<std::uint32_t, 16> local = state;

	for ( std::size_t i = 0; i < Rounds; i += 2 )
	{
		// Odd round
		quarter_round( local[ 0 ], local[ 4 ], local[ 8 ], local[ 12 ] );  // column 1
		quarter_round( local[ 1 ], local[ 5 ], local[ 9 ], local[ 13 ] );  // column 2
		quarter_round( local[ 2 ], local[ 6 ], local[ 10 ], local[ 14 ] ); // column 3
		quarter_round( local[ 3 ], local[ 7 ], local[ 11 ], local[ 15 ] ); // column 4
		// Even round
		quarter_round( local[ 0 ], local[ 5 ], local[ 10 ], local[ 15 ] ); // diagonal 1 (main diagonal)
		quarter_round( local[ 1 ], local[ 6 ], local[ 11 ], local[ 12 ] ); // diagonal 2
		quarter_round( local[ 2 ], local[ 7 ], local[ 8 ], local[ 13 ] );  // diagonal 3
		quarter_round( local[ 3 ], local[ 4 ], local[ 9 ], local[ 14 ] );  // diagonal 4
	}

	for ( std::size_t i = 0; i < 16; ++i )
	{
		local[ i ] += state[ i ];
	}

	for ( std::size_t i = 0; i < keystream_max; ++i )
	{
		keystream[ i ] =
			static_cast<result_type>( local[ 2 * i ] ) | ( static_cast<result_type>( local[ 2 * i + 1 ] ) << 32 );
	}

	++state[ 12 ]; // Increment counter for next block
	keystream_index = 0;
}

template <std::size_t Rounds>
bool mclo::chacha<Rounds>::operator==( const chacha<Rounds>& other ) const noexcept
{
	// Equality is defined by the future keystream, so already consumed buffer entries are ignored.
	return state == other.state && keystream_index == other.keystream_index &&
		   std::equal(
			   keystream.begin() + keystream_index, keystream.end(), other.keystream.begin() + keystream_index );
}

template class mclo::chacha<8>;
template class mclo::chacha<12>;
template class mclo::chacha<20>;
