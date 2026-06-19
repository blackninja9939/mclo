#include <catch2/catch_test_macros.hpp>

#include "mclo/random/chacha.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace
{
	using key_type = std::array<std::uint8_t, 32>;
	using nonce_type = std::array<std::uint8_t, 12>;

	constexpr key_type zero_key{};
	constexpr nonce_type zero_nonce{};

	// RFC 8439 Section 2.3.2 sample key and nonce
	constexpr key_type rfc_key{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
								0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
								0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f };
	constexpr nonce_type rfc_nonce{ 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00 };

	// RFC 8439 Section 2.3.2 keystream, which is generated at block counter 1 for the sample key and nonce.
	constexpr std::array<std::uint8_t, 64> rfc_block1_keystream{
		0x10, 0xf1, 0xe7, 0xe4, 0xd1, 0x3b, 0x59, 0x15, 0x50, 0x0f, 0xdd, 0x1f, 0xa3, 0x20, 0x71, 0xc4,
		0xc7, 0xd1, 0xf4, 0xc7, 0x33, 0xc0, 0x68, 0x03, 0x04, 0x22, 0xaa, 0x9a, 0xc3, 0xd4, 0x6c, 0x4e,
		0xd2, 0x82, 0x64, 0x46, 0x07, 0x9f, 0xaa, 0x09, 0x14, 0xc2, 0xd7, 0x05, 0xd9, 0x8b, 0x02, 0xa2,
		0xb5, 0x12, 0x9c, 0xd1, 0xde, 0x16, 0x4e, 0xb9, 0xcb, 0xd0, 0x83, 0xe8, 0xa2, 0x50, 0x3c, 0x4e };

	// Assembles a 64 byte keystream block into the 8 little endian 64 bit words the engine produces, matching the
	// engine's serialization so the comparison is independent of host endianness.
	[[nodiscard]] std::array<std::uint64_t, 8> keystream_to_words( const std::array<std::uint8_t, 64>& bytes ) noexcept
	{
		std::array<std::uint64_t, 8> words{};
		for ( std::size_t i = 0; i < words.size(); ++i )
		{
			for ( std::size_t j = 0; j < sizeof( std::uint64_t ); ++j )
			{
				words[ i ] |= static_cast<std::uint64_t>( bytes[ i * sizeof( std::uint64_t ) + j ] ) << ( j * 8 );
			}
		}
		return words;
	}

	template <std::size_t Rounds>
	[[nodiscard]] std::array<std::uint64_t, 8> next_block( mclo::chacha<Rounds>& engine ) noexcept
	{
		std::array<std::uint64_t, 8> block{};
		for ( std::uint64_t& word : block )
		{
			word = engine();
		}
		return block;
	}

	static_assert( std::uniform_random_bit_generator<mclo::chacha20>,
				   "chacha20 should model the uniform random bit generator concept" );
}

TEST_CASE( "chacha20 zero key and nonce, first block, matches RFC 8439 block counter zero keystream", "[random]" )
{
	// Given - RFC 8439 Appendix A.1 Test Vector #1 (all zero key and nonce, block counter 0)
	constexpr std::array<std::uint8_t, 64> expected_keystream{
		0x76, 0xb8, 0xe0, 0xad, 0xa0, 0xf1, 0x3d, 0x90, 0x40, 0x5d, 0x6a, 0xe5, 0x53, 0x86, 0xbd, 0x28,
		0xbd, 0xd2, 0x19, 0xb8, 0xa0, 0x8d, 0xed, 0x1a, 0xa8, 0x36, 0xef, 0xcc, 0x8b, 0x77, 0x0d, 0xc7,
		0xda, 0x41, 0x59, 0x7c, 0x51, 0x57, 0x48, 0x8d, 0x77, 0x24, 0xe0, 0x3f, 0xb8, 0xd8, 0x4a, 0x37,
		0x6a, 0x43, 0xb8, 0xf4, 0x15, 0x18, 0xa1, 0x1c, 0xc3, 0x87, 0xb6, 0x69, 0xb2, 0xee, 0x65, 0x86 };

	mclo::chacha20 engine( zero_key, zero_nonce );

	// When - generating the first block (block counter 0)
	const std::array<std::uint64_t, 8> block = next_block( engine );

	// Then - it matches the published keystream
	CHECK( block == keystream_to_words( expected_keystream ) );
}

TEST_CASE( "chacha8 zero key and nonce, first block, matches the published reduced round keystream", "[random]" )
{
	// Given - draft-strombergson-chacha-test-vectors-01 TC1 (all zero 256 bit key and IV) for 8 rounds. At block 0
	// with a zero nonce the original 64 bit counter / IV layout and the RFC 8439 layout share the same input state.
	constexpr std::array<std::uint8_t, 64> expected_keystream{
		0x3e, 0x00, 0xef, 0x2f, 0x89, 0x5f, 0x40, 0xd6, 0x7f, 0x5b, 0xb8, 0xe8, 0x1f, 0x09, 0xa5, 0xa1,
		0x2c, 0x84, 0x0e, 0xc3, 0xce, 0x9a, 0x7f, 0x3b, 0x18, 0x1b, 0xe1, 0x88, 0xef, 0x71, 0x1a, 0x1e,
		0x98, 0x4c, 0xe1, 0x72, 0xb9, 0x21, 0x6f, 0x41, 0x9f, 0x44, 0x53, 0x67, 0x45, 0x6d, 0x56, 0x19,
		0x31, 0x4a, 0x42, 0xa3, 0xda, 0x86, 0xb0, 0x01, 0x38, 0x7b, 0xfd, 0xb8, 0x0e, 0x0c, 0xfe, 0x42 };

	mclo::chacha8 engine( zero_key, zero_nonce );

	// When - generating the first block (block counter 0)
	const std::array<std::uint64_t, 8> block = next_block( engine );

	// Then - it matches the published keystream
	CHECK( block == keystream_to_words( expected_keystream ) );
}

TEST_CASE( "chacha12 zero key and nonce, first block, matches the published reduced round keystream", "[random]" )
{
	// Given - draft-strombergson-chacha-test-vectors-01 TC1 (all zero 256 bit key and IV) for 12 rounds. At block 0
	// with a zero nonce the original 64 bit counter / IV layout and the RFC 8439 layout share the same input state.
	constexpr std::array<std::uint8_t, 64> expected_keystream{
		0x9b, 0xf4, 0x9a, 0x6a, 0x07, 0x55, 0xf9, 0x53, 0x81, 0x1f, 0xce, 0x12, 0x5f, 0x26, 0x83, 0xd5,
		0x04, 0x29, 0xc3, 0xbb, 0x49, 0xe0, 0x74, 0x14, 0x7e, 0x00, 0x89, 0xa5, 0x2e, 0xae, 0x15, 0x5f,
		0x05, 0x64, 0xf8, 0x79, 0xd2, 0x7a, 0xe3, 0xc0, 0x2c, 0xe8, 0x28, 0x34, 0xac, 0xfa, 0x8c, 0x79,
		0x3a, 0x62, 0x9f, 0x2c, 0xa0, 0xde, 0x69, 0x19, 0x61, 0x0b, 0xe8, 0x2f, 0x41, 0x13, 0x26, 0xbe };

	mclo::chacha12 engine( zero_key, zero_nonce );

	// When - generating the first block (block counter 0)
	const std::array<std::uint64_t, 8> block = next_block( engine );

	// Then - it matches the published keystream
	CHECK( block == keystream_to_words( expected_keystream ) );
}

TEST_CASE( "chacha20 RFC sample key and nonce, second block, matches RFC 8439 block counter one keystream", "[random]" )
{
	// Given - RFC 8439 Section 2.3.2 keystream is for block counter 1, so the first block (counter 0) is discarded
	mclo::chacha20 engine( rfc_key, rfc_nonce );

	// When - discarding block counter 0 then generating block counter 1
	engine.discard( 8 );
	const std::array<std::uint64_t, 8> block = next_block( engine );

	// Then - it matches the published keystream
	CHECK( block == keystream_to_words( rfc_block1_keystream ) );
}

TEST_CASE( "chacha20 set_counter to one, next block, matches RFC 8439 block counter one keystream", "[random]" )
{
	// Given
	mclo::chacha20 engine( rfc_key, rfc_nonce );

	// When - seeking directly to block counter 1
	engine.set_counter( 1 );
	const std::array<std::uint64_t, 8> block = next_block( engine );

	// Then - it matches the same published keystream as discarding the first block
	CHECK( block == keystream_to_words( rfc_block1_keystream ) );
}

TEST_CASE( "chacha20 get_counter reports the block advancing as results are consumed", "[random]" )
{
	// Given
	mclo::chacha20 engine( rfc_key, rfc_nonce );
	CHECK( engine.get_counter() == 0 );

	// When - consuming one full block (8 results)
	( void )next_block( engine );

	// Then - the counter advanced to the next block
	CHECK( engine.get_counter() == 1 );
}

TEST_CASE( "chacha20 set_counter seeks to the same position as repeated discards", "[random]" )
{
	// Given - one engine seeks by counter, the other by discarding whole blocks
	mclo::chacha20 seeked( rfc_key, rfc_nonce );
	mclo::chacha20 discarded( rfc_key, rfc_nonce );

	// When
	seeked.set_counter( 5 );
	discarded.discard( 5 * 8 );

	// Then - both are at the same position and compare equal
	CHECK( seeked == discarded );
	CHECK( next_block( seeked ) == next_block( discarded ) );
}

TEST_CASE( "chacha20 min and max return the result_type limits", "[random]" )
{
	mclo::chacha20 engine( rfc_key, rfc_nonce );

	CHECK( engine.min() == std::numeric_limits<mclo::chacha20::result_type>::min() );
	CHECK( engine.max() == std::numeric_limits<mclo::chacha20::result_type>::max() );
}

TEST_CASE( "chacha20 consecutive calls advance the keystream", "[random]" )
{
	// Given
	mclo::chacha20 engine( rfc_key, rfc_nonce );

	// When - crossing a block boundary (8 results per block)
	const std::array<std::uint64_t, 8> first_block = next_block( engine );
	const std::array<std::uint64_t, 8> second_block = next_block( engine );

	// Then - the blocks differ, confirming the counter advanced
	CHECK( first_block != second_block );
}

TEST_CASE( "chacha20 discard skips the same values as repeated calls", "[random]" )
{
	// Given - two identically seeded engines
	mclo::chacha20 discarded( rfc_key, rfc_nonce );
	mclo::chacha20 stepped( rfc_key, rfc_nonce );

	// When - one discards a count that crosses several block boundaries while the other consumes the same count
	constexpr int skip = 21; // not a multiple of the 8 results per block
	discarded.discard( skip );
	for ( int i = 0; i < skip; ++i )
	{
		( void )stepped();
	}

	// Then - both engines are equal and produce the same subsequent values
	CHECK( discarded == stepped );
	CHECK( discarded() == stepped() );
	CHECK( discarded() == stepped() );
}

TEST_CASE( "chacha20 discard within the current block matches repeated calls", "[random]" )
{
	// Given - two engines partway through the first block
	mclo::chacha20 discarded( rfc_key, rfc_nonce );
	mclo::chacha20 stepped( rfc_key, rfc_nonce );
	( void )discarded();
	( void )stepped();

	// When - discarding fewer values than remain in the current block
	discarded.discard( 3 );
	for ( int i = 0; i < 3; ++i )
	{
		( void )stepped();
	}

	// Then - both remain equal
	CHECK( discarded == stepped );
	CHECK( discarded() == stepped() );
}

TEST_CASE( "chacha20 reseeding restarts the keystream", "[random]" )
{
	// Given
	mclo::chacha20 engine( rfc_key, rfc_nonce );
	const std::array<std::uint64_t, 8> initial = next_block( engine );

	// When - advancing then reseeding with the same key and nonce
	engine.discard( 32 );
	engine.seed( rfc_key, rfc_nonce );

	// Then - the keystream restarts
	CHECK( next_block( engine ) == initial );
}

TEST_CASE( "chacha20 engines with the same seed compare equal", "[random]" )
{
	mclo::chacha20 lhs( rfc_key, rfc_nonce );
	mclo::chacha20 rhs( rfc_key, rfc_nonce );

	CHECK( lhs == rhs );
	CHECK_FALSE( lhs != rhs );
}

TEST_CASE( "chacha20 engines with different nonces compare unequal", "[random]" )
{
	nonce_type other_nonce = rfc_nonce;
	other_nonce[ 0 ] ^= 0x01;

	mclo::chacha20 lhs( rfc_key, rfc_nonce );
	mclo::chacha20 rhs( rfc_key, other_nonce );

	CHECK( lhs != rhs );
	CHECK_FALSE( lhs == rhs );
}

TEST_CASE( "chacha20 engines at different keystream positions compare unequal", "[random]" )
{
	mclo::chacha20 lhs( rfc_key, rfc_nonce );
	mclo::chacha20 rhs( rfc_key, rfc_nonce );

	( void )lhs();

	CHECK( lhs != rhs );
	CHECK_FALSE( lhs == rhs );
}

TEST_CASE( "chacha20 satisfies a uniform integer distribution range", "[random]" )
{
	mclo::chacha20 engine( rfc_key, rfc_nonce );
	std::uniform_int_distribution<int> distribution( 0, 100 );

	std::size_t count = 100;
	while ( count-- )
	{
		const int value = distribution( engine );
		// The distribution's exact implementation is not specified, so we can only check the value is in range.
		CHECK( value >= 0 );
		CHECK( value <= 100 );
	}
}
