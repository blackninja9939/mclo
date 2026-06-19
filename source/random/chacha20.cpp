#include "mclo/random/chacha20.hpp"

#include "mclo/numeric/bit.hpp"

#include <algorithm>
#include <cstring>

#include <xsimd/xsimd.hpp>

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

	using row = xsimd::batch<std::uint32_t, xsimd::sse2>;

	void quarter_round( row& a, row& b, row& c, row& d ) noexcept
	{
		a += b;
		d = xsimd::rotl( d ^ a, 16 );

		c += d;
		b = xsimd::rotl( b ^ c, 12 );

		a += b;
		d = xsimd::rotl( d ^ a, 8 );

		c += d;
		b = xsimd::rotl( b ^ c, 7 );
	}
}

template <std::size_t Rounds>
mclo::chacha<Rounds>::chacha( const std::array<std::uint8_t, 32>& seed,
							  const std::array<std::uint8_t, 12>& nonce ) noexcept
	: state{ 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 } // "expand 32-byte k" constants
{
	static_assert( alignof( mclo::chacha<Rounds> ) == row::arch_type::alignment(), "chacha must be aligned for sse2" );
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
	const row state_a = row::load_aligned( state.data() );
	const row state_b = row::load_aligned( state.data() + 4 );
	const row state_c = row::load_aligned( state.data() + 8 );
	const row state_d = row::load_aligned( state.data() + 12 );

	row a = state_a;
	row b = state_b;
	row c = state_c;
	row d = state_d;

	for ( std::size_t i = 0; i < Rounds; i += 2 )
	{
		// Column rounds
		quarter_round( a, b, c, d );

		// Rotate for diagonal rounds
		b = xsimd::rotate_left<1>( b );
		c = xsimd::rotate_left<2>( c );
		d = xsimd::rotate_left<3>( d );

		// Diagonal rounds
		quarter_round( a, b, c, d );

		// Rotate back after diagonal rounds
		b = xsimd::rotate_right<1>( b );
		c = xsimd::rotate_right<2>( c );
		d = xsimd::rotate_right<3>( d );
	}

	a += state_a;
	b += state_b;
	c += state_c;
	d += state_d;

	alignas( row::arch_type::alignment() ) std::array<std::uint32_t, 16> local;
	a.store_aligned( local.data() );
	b.store_aligned( local.data() + 4 );
	c.store_aligned( local.data() + 8 );
	d.store_aligned( local.data() + 12 );

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
