#include "mclo/hash/murmur_hash_3.hpp"

#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <limits>

// Look at end of the file for the hasher using the open source code copied here

/*-----------------------------------------------------------------------------
 * MurmurHash3 was written by Austin Appleby, and is placed in the public
 * domain.
 *
 * This implementation was written by Shane Day, and is also public domain.
 *
 * This is a portable ANSI C implementation of MurmurHash3_x86_32 (Murmur3A)
 * with support for progressive processing.
 */

/*-----------------------------------------------------------------------------

If you want to understand the MurmurHash algorithm you would be much better
off reading the original source. Just point your browser at:
http://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp


What this version provides?

1. Progressive data feeding. Useful when the entire payload to be hashed
does not fit in memory or when the data is streamed through the application.
Also useful when hashing a number of strings with a common prefix. A partial
hash of a prefix string can be generated and reused for each suffix string.

2. Portability. Plain old C so that it should compile on any old compiler.
Both CPU endian and access-alignment neutral, but avoiding inefficient code
when possible depending on CPU capabilities.

3. Drop in. I personally like nice self contained public domain code, making it
easy to pilfer without loads of refactoring to work properly in the existing
application code & makefile structure and mucking around with licence files.
Just copy PMurHash.h and PMurHash.c and you're ready to go.


How does it work?

We can only process entire 32 bit chunks of input, except for the very end
that may be shorter. So along with the partial hash we need to give back to
the caller a carry containing up to 3 bytes that we were unable to process.
This carry also needs to record the number of bytes the carry holds. I use
the low 2 bits as a count (0..3) and the carry bytes are shifted into the
high byte in stream order.

To handle endianess I simply use a macro that reads a uint32_t and define
that macro to be a direct read on little endian machines, a read and swap
on big endian machines, or a byte-by-byte read if the endianess is unknown.

-----------------------------------------------------------------------------*/

/* MSVC warnings we choose to ignore */
#if defined( _MSC_VER )
#pragma warning( disable : 4127 ) /* conditional expression is constant */
#endif

/*-----------------------------------------------------------------------------
 * Endianess, misalignment capabilities and util macros
 *
 * The following 3 macros are defined in this section. The other macros defined
 * are only needed to help derive these 3.
 *
 * READ_UINT32(x)   Read a little endian unsigned 32-bit int
 * UNALIGNED_SAFE   Defined if READ_UINT32 works on non-word boundaries
 * ROTL32(x,r)      Rotate x left by r bits
 */

/* Convention is to define __BYTE_ORDER == to one of these values */
#if !defined( __BIG_ENDIAN )
#define __BIG_ENDIAN 4321
#endif
#if !defined( __LITTLE_ENDIAN )
#define __LITTLE_ENDIAN 1234
#endif

/* I386 */
#if defined( _M_IX86 ) || defined( __i386__ ) || defined( __i386 ) || defined( i386 )
#define __BYTE_ORDER __LITTLE_ENDIAN
#define UNALIGNED_SAFE
#endif

/* gcc 'may' define __LITTLE_ENDIAN__ or __BIG_ENDIAN__ to 1 (Note the trailing __),
 * or even _LITTLE_ENDIAN or _BIG_ENDIAN (Note the single _ prefix) */
#if !defined( __BYTE_ORDER )
#if defined( __LITTLE_ENDIAN__ ) && __LITTLE_ENDIAN__ == 1 || defined( _LITTLE_ENDIAN ) && _LITTLE_ENDIAN == 1
#define __BYTE_ORDER __LITTLE_ENDIAN
#elif defined( __BIG_ENDIAN__ ) && __BIG_ENDIAN__ == 1 || defined( _BIG_ENDIAN ) && _BIG_ENDIAN == 1
#define __BYTE_ORDER __BIG_ENDIAN
#endif
#endif

/* gcc (usually) defines xEL/EB macros for ARM and MIPS endianess */
#if !defined( __BYTE_ORDER )
#if defined( __ARMEL__ ) || defined( __MIPSEL__ )
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#if defined( __ARMEB__ ) || defined( __MIPSEB__ )
#define __BYTE_ORDER __BIG_ENDIAN
#endif
#endif

/* Now find best way we can to READ_UINT32 */
#if __BYTE_ORDER == __LITTLE_ENDIAN
/* CPU endian matches murmurhash algorithm, so read 32-bit word directly */
#define READ_UINT32( ptr ) ( *( ( uint32_t* )( ptr ) ) )
#elif __BYTE_ORDER == __BIG_ENDIAN
/* TODO: Add additional cases below where a compiler provided bswap32 is available */
#if defined( __GNUC__ ) && ( __GNUC__ > 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ >= 3 ) )
#define READ_UINT32( ptr ) ( __builtin_bswap32( *( ( std::uint32_t* )( ptr ) ) ) )
#else
/* Without a known fast bswap32 we're just as well off doing this */
#define READ_UINT32( ptr ) ( ptr[ 0 ] | ptr[ 1 ] << 8 | ptr[ 2 ] << 16 | ptr[ 3 ] << 24 )
#define UNALIGNED_SAFE
#endif
#else
/* Unknown endianess so last resort is to read individual bytes */
#define READ_UINT32( ptr ) ( ptr[ 0 ] | ptr[ 1 ] << 8 | ptr[ 2 ] << 16 | ptr[ 3 ] << 24 )

/* Since we're not doing word-reads we can skip the messing about with realignment */
#define UNALIGNED_SAFE
#endif

/* Find best way to ROTL32 */
#if defined( _MSC_VER )
#include <stdlib.h> /* Microsoft put _rotl declaration in here */
#define ROTL32( x, r ) _rotl( x, r )
#else
/* gcc recognises this code and generates a rotate instruction for CPUs with one */
#define ROTL32( x, r ) ( ( ( std::uint32_t )x << r ) | ( ( std::uint32_t )x >> ( 32 - r ) ) )
#endif

/*-----------------------------------------------------------------------------
 * Core murmurhash algorithm macros */

#define C1 ( 0xcc9e2d51 )
#define C2 ( 0x1b873593 )

/* This is the main processing body of the algorithm. It operates
 * on each full 32-bits of input. */
#define DOBLOCK( h1, k1 )                                                                                              \
	do                                                                                                                 \
	{                                                                                                                  \
		k1 *= C1;                                                                                                      \
		k1 = ROTL32( k1, 15 );                                                                                         \
		k1 *= C2;                                                                                                      \
                                                                                                                       \
		h1 ^= k1;                                                                                                      \
		h1 = ROTL32( h1, 13 );                                                                                         \
		h1 = h1 * 5 + 0xe6546b64;                                                                                      \
	}                                                                                                                  \
	while ( 0 )

/* Append unaligned bytes to carry, forcing hash churn if we have 4 bytes */
/* cnt=bytes to process, h1=name of h1 var, c=carry, n=bytes in c, ptr/len=payload */
#define DOBYTES( cnt, h1, c, n, ptr, len )                                                                             \
	do                                                                                                                 \
	{                                                                                                                  \
		std::size_t _i = cnt;                                                                                          \
		while ( _i-- )                                                                                                 \
		{                                                                                                              \
			c = c >> 8 | *ptr++ << 24;                                                                                 \
			n++;                                                                                                       \
			len--;                                                                                                     \
			if ( n == 4 )                                                                                              \
			{                                                                                                          \
				DOBLOCK( h1, c );                                                                                      \
				n = 0;                                                                                                 \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	while ( 0 )

/*---------------------------------------------------------------------------*/

/* Main hashing function. Initialise carry to 0 and h1 to 0 or an initial seed
 * if wanted. Both ph1 and pcarry are required arguments. */
static void PMurHash32_Process( std::uint32_t* const MCLO_RESTRICT ph1,
								std::uint32_t* const MCLO_RESTRICT pcarry,
								const std::uint8_t* key,
								std::size_t len )
{
	std::uint32_t h1 = *ph1;
	std::uint32_t c = *pcarry;

	const std::uint8_t* ptr = key;
	const std::uint8_t* end;

	/* Extract carry count from low 2 bits of c value */
	int n = c & 3;

#if defined( UNALIGNED_SAFE )
	/* This CPU handles unaligned word access */

	/* Consume any carry bytes */
	std::size_t i = ( 4 - n ) & 3;
	if ( i && i <= len )
	{
		DOBYTES( i, h1, c, n, ptr, len );
	}

	/* Process 32-bit chunks */
	end = ptr + len / 4 * 4;
	for ( ; ptr < end; ptr += 4 )
	{
		std::uint32_t k1 = READ_UINT32( ptr );
		DOBLOCK( h1, k1 );
	}

#else  /*UNALIGNED_SAFE*/
	/* This CPU does not handle unaligned word access */

	/* Consume enough so that the next data byte is word aligned */
	std::size_t i = -( long )ptr & 3;
	if ( i && i <= len )
	{
		DOBYTES( i, h1, c, n, ptr, len );
	}

	/* We're now aligned. Process in aligned blocks. Specialise for each possible carry count */
	end = ptr + len / 4 * 4;
	switch ( n )
	{           /* how many bytes in c */
		case 0: /* c=[----]  w=[3210]  b=[3210]=w            c'=[----] */
			for ( ; ptr < end; ptr += 4 )
			{
				std::uint32_t k1 = READ_UINT32( ptr );
				DOBLOCK( h1, k1 );
			}
			break;
		case 1: /* c=[0---]  w=[4321]  b=[3210]=c>>24|w<<8   c'=[4---] */
			for ( ; ptr < end; ptr += 4 )
			{
				std::uint32_t k1 = c >> 24;
				c = READ_UINT32( ptr );
				k1 |= c << 8;
				DOBLOCK( h1, k1 );
			}
			break;
		case 2: /* c=[10--]  w=[5432]  b=[3210]=c>>16|w<<16  c'=[54--] */
			for ( ; ptr < end; ptr += 4 )
			{
				std::uint32_t k1 = c >> 16;
				c = READ_UINT32( ptr );
				k1 |= c << 16;
				DOBLOCK( h1, k1 );
			}
			break;
		case 3: /* c=[210-]  w=[6543]  b=[3210]=c>>8|w<<24   c'=[654-] */
			for ( ; ptr < end; ptr += 4 )
			{
				std::uint32_t k1 = c >> 8;
				c = READ_UINT32( ptr );
				k1 |= c << 24;
				DOBLOCK( h1, k1 );
			}
	}
#endif /*UNALIGNED_SAFE*/

	/* Advance over whole 32-bit chunks, possibly leaving 1..3 bytes */
	len -= len / 4 * 4;

	/* Append any remaining bytes into carry */
	DOBYTES( len, h1, c, n, ptr, len );

	/* Copy out new running hash and carry */
	*ph1 = h1;
	*pcarry = ( c & ~0xff ) | n;
}

/*---------------------------------------------------------------------------*/

/* Finalize a hash. To match the original Murmur3A the total_length must be provided */
static std::uint32_t PMurHash32_Result( std::uint32_t h, std::uint32_t carry, std::uint32_t total_length )
{
	std::uint32_t k1;
	std::size_t n = carry & 3;
	if ( n )
	{
		k1 = carry >> ( 4 - n ) * 8;
		k1 *= C1;
		k1 = ROTL32( k1, 15 );
		k1 *= C2;
		h ^= k1;
	}
	h ^= total_length;

	/* fmix */
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

void mclo::murmur_hash_3::write( const mclo::span<const std::byte> data ) noexcept
{
	PMurHash32_Process( &m_hash, &m_carry, reinterpret_cast<const std::uint8_t*>( data.data() ), data.size() );
	DEBUG_ASSERT( std::numeric_limits<std::uint32_t>::max() - m_total_length >= data.size(),
				  "MurmurHash3 can only process up to uint32_t max data" );
	m_total_length += static_cast<std::uint32_t>( data.size() );
}

std::size_t mclo::murmur_hash_3::finish() noexcept
{
	return PMurHash32_Result( m_hash, m_carry, m_total_length );
}
