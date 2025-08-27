#include "mclo/hash/rapidhash.hpp"

#include "mclo/debug/assert.hpp"
#include "mclo/preprocessor/platform.hpp"

#include <limits>

/*
Copyright 2025 Nicolas De Carli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Modified by Matthew Clohessy to fit into my streaming API somewhat questionably

#include <stdint.h>
#include <string.h>
#if defined( _MSC_VER )
#include <intrin.h>
#if defined( _M_X64 ) && !defined( _M_ARM64EC )
#pragma intrinsic( _umul128 )
#endif
#endif

/*
 *  C++ macros.
 *
 *  RAPIDHASH_INLINE can be overridden to be stronger than a hint, i.e. by adding __attribute__((always_inline)).
 */
#ifdef __cplusplus
#define RAPIDHASH_NOEXCEPT noexcept
#define RAPIDHASH_CONSTEXPR constexpr
#ifndef RAPIDHASH_INLINE
#define RAPIDHASH_INLINE inline
#endif
#else
#define RAPIDHASH_NOEXCEPT
#define RAPIDHASH_CONSTEXPR static const
#ifndef RAPIDHASH_INLINE
#define RAPIDHASH_INLINE static inline
#endif
#endif

/*
 *  Protection macro, alters behaviour of rapid_mum multiplication function.
 *
 *  RAPIDHASH_FAST: Normal behavior, max speed.
 *  RAPIDHASH_PROTECTED: Extra protection against entropy loss.
 */
#ifndef RAPIDHASH_PROTECTED
#define RAPIDHASH_FAST
#elif defined( RAPIDHASH_FAST )
#error "cannot define RAPIDHASH_PROTECTED and RAPIDHASH_FAST simultaneously."
#endif

/*
 *  Unrolling macros, changes code definition for main hash function.
 *
 *  RAPIDHASH_COMPACT: Legacy variant, each loop process 48 bytes.
 *  RAPIDHASH_UNROLLED: Unrolled variant, each loop process 96 bytes.
 *
 *  Most modern CPUs should benefit from having RAPIDHASH_UNROLLED.
 *
 *  These macros do not alter the output hash.
 */
#ifndef RAPIDHASH_COMPACT
#define RAPIDHASH_UNROLLED
#elif defined( RAPIDHASH_UNROLLED )
#error "cannot define RAPIDHASH_COMPACT and RAPIDHASH_UNROLLED simultaneously."
#endif

/*
 *  Endianness macros.
 */
#ifndef RAPIDHASH_LITTLE_ENDIAN
#if defined( _WIN32 ) || defined( __LITTLE_ENDIAN__ ) ||                                                               \
	( defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ )
#define RAPIDHASH_LITTLE_ENDIAN
#elif defined( __BIG_ENDIAN__ ) || ( defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ )
#define RAPIDHASH_BIG_ENDIAN
#else
#warning "could not determine endianness! Falling back to little endian."
#define RAPIDHASH_LITTLE_ENDIAN
#endif
#endif

namespace
{
	RAPIDHASH_CONSTEXPR uint64_t RAPID_SECRET[ 3 ] = {
		0x2d358dccaa6c78a5ull, 0x8bb84b93962eacc9ull, 0x4b33a62ed433d4a3ull };

	RAPIDHASH_INLINE void rapid_mum( uint64_t* MCLO_RESTRICT A, uint64_t* MCLO_RESTRICT B ) RAPIDHASH_NOEXCEPT
	{
#if defined( __SIZEOF_INT128__ )
		__uint128_t r = *A;
		r *= *B;
#ifdef RAPIDHASH_PROTECTED
		*A ^= ( uint64_t )r;
		*B ^= ( uint64_t )( r >> 64 );
#else
		*A = ( uint64_t )r;
		*B = ( uint64_t )( r >> 64 );
#endif
#elif defined( _MSC_VER ) && ( defined( _WIN64 ) || defined( _M_HYBRID_CHPE_ARM64 ) )
#if defined( _M_X64 )
#ifdef RAPIDHASH_PROTECTED
		uint64_t m_a, m_b;
		m_a = _umul128( *A, *B, &m_b );
		*A ^= m_a;
		*B ^= m_b;
#else
		*A = _umul128( *A, *B, B );
#endif
#else
#ifdef RAPIDHASH_PROTECTED
		uint64_t m_a, m_b;
		m_b = __umulh( *A, *B );
		m_a = *A * *B;
		*A ^= m_a;
		*B ^= m_b;
#else
		uint64_t c = __umulh( *A, *B );
		*A = *A * *B;
		*B = c;
#endif
#endif
#else
		uint64_t ha = *A >> 32, hb = *B >> 32, la = ( uint32_t )*A, lb = ( uint32_t )*B, hi, lo;
		uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb, t = rl + ( rm0 << 32 ), c = t < rl;
		lo = t + ( rm1 << 32 );
		c += lo < t;
		hi = rh + ( rm0 >> 32 ) + ( rm1 >> 32 ) + c;
#ifdef RAPIDHASH_PROTECTED
		*A ^= lo;
		*B ^= hi;
#else
		*A = lo;
		*B = hi;
#endif
#endif
	}

	/*
	 *  Multiply and xor mix function.
	 *
	 *  @param A  64-bit number.
	 *  @param B  64-bit number.
	 *
	 *  Calculates 128-bit C = A * B.
	 *  Returns 64-bit xor between high and low 64 bits of C.
	 */
	RAPIDHASH_INLINE uint64_t rapid_mix( uint64_t A, uint64_t B ) RAPIDHASH_NOEXCEPT
	{
		rapid_mum( &A, &B );
		return A ^ B;
	}

	RAPIDHASH_INLINE std::uint64_t rapidhash_seed( uint64_t seed, uint64_t len )
	{
		return seed ^ rapid_mix( seed ^ RAPID_SECRET[ 0 ], RAPID_SECRET[ 1 ] ) ^ len;
	}

	/*
	 *  Read functions.
	 */
#ifdef RAPIDHASH_LITTLE_ENDIAN
	RAPIDHASH_INLINE uint64_t rapid_read64( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint64_t v;
		std::memcpy( &v, p, sizeof( uint64_t ) );
		return v;
	}
	RAPIDHASH_INLINE uint64_t rapid_read32( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint32_t v;
		std::memcpy( &v, p, sizeof( uint32_t ) );
		return v;
	}
#elif defined( __GNUC__ ) || defined( __INTEL_COMPILER ) || defined( __clang__ )
	RAPIDHASH_INLINE uint64_t rapid_read64( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint64_t v;
		std::memcpy( &v, p, sizeof( uint64_t ) );
		return __builtin_bswap64( v );
	}
	RAPIDHASH_INLINE uint64_t rapid_read32( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint32_t v;
		std::memcpy( &v, p, sizeof( uint32_t ) );
		return __builtin_bswap32( v );
	}
#elif defined( _MSC_VER )
	RAPIDHASH_INLINE uint64_t rapid_read64( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint64_t v;
		std::memcpy( &v, p, sizeof( uint64_t ) );
		return _byteswap_uint64( v );
	}
	RAPIDHASH_INLINE uint64_t rapid_read32( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint32_t v;
		std::memcpy( &v, p, sizeof( uint32_t ) );
		return _byteswap_ulong( v );
	}
#else
	RAPIDHASH_INLINE uint64_t rapid_read64( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint64_t v;
		std::memcpy( &v, p, 8 );
		return ( ( ( v >> 56 ) & 0xff ) | ( ( v >> 40 ) & 0xff00 ) | ( ( v >> 24 ) & 0xff0000 ) |
				 ( ( v >> 8 ) & 0xff000000 ) | ( ( v << 8 ) & 0xff00000000 ) | ( ( v << 24 ) & 0xff0000000000 ) |
				 ( ( v << 40 ) & 0xff000000000000 ) | ( ( v << 56 ) & 0xff00000000000000 ) );
	}
	RAPIDHASH_INLINE uint64_t rapid_read32( const std::byte* p ) RAPIDHASH_NOEXCEPT
	{
		uint32_t v;
		std::memcpy( &v, p, 4 );
		return ( ( ( v >> 24 ) & 0xff ) | ( ( v >> 8 ) & 0xff00 ) | ( ( v << 8 ) & 0xff0000 ) |
				 ( ( v << 24 ) & 0xff000000 ) );
	}
#endif

	/*
	 *  Reads and combines 3 bytes of input.
	 *
	 *  @param p  Buffer to read from.
	 *  @param k  Length of @p, in bytes.
	 *
	 *  Always reads and combines 3 bytes from memory.
	 *  Guarantees to read each buffer position at least once.
	 *
	 *  Returns a 64-bit value containing all three bytes read.
	 */
	RAPIDHASH_INLINE uint64_t rapid_readSmall( const std::byte* p, size_t k ) RAPIDHASH_NOEXCEPT
	{
		return ( ( ( uint64_t )p[ 0 ] ) << 56 ) | ( ( ( uint64_t )p[ k >> 1 ] ) << 32 ) |
			   std::to_integer<std::uint8_t>( p[ k - 1 ] );
	}
}

namespace mclo
{
	rapidhash::rapidhash( const std::uint64_t seed ) noexcept
		: m_seed( seed )
	{
	}

	void rapidhash::write( const mclo::span<const std::byte> data ) noexcept
	{
		const std::byte* p = data.data();
		const std::size_t len = data.size();
		m_size += len;

		m_seed ^= rapid_mix( m_seed ^ RAPID_SECRET[ 0 ], RAPID_SECRET[ 1 ] ) ^ len;

		if ( len <= 16 ) [[likely]]
		{
			if ( len >= 4 ) [[likely]]
			{
				const std::byte* plast = p + len - 4;
				m_a = ( rapid_read32( p ) << 32 ) | rapid_read32( plast );
				const uint64_t delta = ( ( len & 24 ) >> ( len >> 3 ) );
				m_b = ( ( rapid_read32( p + delta ) << 32 ) | rapid_read32( plast - delta ) );
			}
			else if ( len > 0 ) [[likely]]
			{
				m_a = rapid_readSmall( p, len );
				m_b = 0;
			}
			else
			{
				m_a = m_b = 0;
			}
		}
		else
		{
			size_t i = len;
			if ( i > 48 ) [[unlikely]]
			{
				uint64_t see1 = m_seed, see2 = m_seed;
#ifdef RAPIDHASH_UNROLLED
				while ( i >= 96 ) [[likely]]
				{
					m_seed = rapid_mix( rapid_read64( p ) ^ RAPID_SECRET[ 0 ], rapid_read64( p + 8 ) ^ m_seed );
					see1 = rapid_mix( rapid_read64( p + 16 ) ^ RAPID_SECRET[ 1 ], rapid_read64( p + 24 ) ^ see1 );
					see2 = rapid_mix( rapid_read64( p + 32 ) ^ RAPID_SECRET[ 2 ], rapid_read64( p + 40 ) ^ see2 );
					m_seed = rapid_mix( rapid_read64( p + 48 ) ^ RAPID_SECRET[ 0 ], rapid_read64( p + 56 ) ^ m_seed );
					see1 = rapid_mix( rapid_read64( p + 64 ) ^ RAPID_SECRET[ 1 ], rapid_read64( p + 72 ) ^ see1 );
					see2 = rapid_mix( rapid_read64( p + 80 ) ^ RAPID_SECRET[ 2 ], rapid_read64( p + 88 ) ^ see2 );
					p += 96;
					i -= 96;
				}
				if ( i >= 48 ) [[unlikely]]
				{
					m_seed = rapid_mix( rapid_read64( p ) ^ RAPID_SECRET[ 0 ], rapid_read64( p + 8 ) ^ m_seed );
					see1 = rapid_mix( rapid_read64( p + 16 ) ^ RAPID_SECRET[ 1 ], rapid_read64( p + 24 ) ^ see1 );
					see2 = rapid_mix( rapid_read64( p + 32 ) ^ RAPID_SECRET[ 2 ], rapid_read64( p + 40 ) ^ see2 );
					p += 48;
					i -= 48;
				}
#else
				[[likely]] do
				{
					m_seed = rapid_mix( rapid_read64( p ) ^ RAPID_SECRET[ 0 ], rapid_read64( p + 8 ) ^ m_seed );
					see1 = rapid_mix( rapid_read64( p + 16 ) ^ RAPID_SECRET[ 1 ], rapid_read64( p + 24 ) ^ see1 );
					see2 = rapid_mix( rapid_read64( p + 32 ) ^ RAPID_SECRET[ 2 ], rapid_read64( p + 40 ) ^ see2 );
					p += 48;
					i -= 48;
				}
				while ( i >= 48 );
#endif
				m_seed ^= see1 ^ see2;
			}
			if ( i > 16 )
			{
				m_seed = rapid_mix( rapid_read64( p ) ^ RAPID_SECRET[ 2 ],
									rapid_read64( p + 8 ) ^ m_seed ^ RAPID_SECRET[ 1 ] );
				if ( i > 32 )
				{
					m_seed = rapid_mix( rapid_read64( p + 16 ) ^ RAPID_SECRET[ 2 ], rapid_read64( p + 24 ) ^ m_seed );
				}
			}
			m_a = rapid_read64( p + i - 16 );
			m_b = rapid_read64( p + i - 8 );
		}

		m_a ^= RAPID_SECRET[ 1 ];
		m_b ^= m_seed;
		rapid_mum( &m_a, &m_b );
	}

	std::size_t rapidhash::finish() noexcept
	{
		return rapid_mix( m_a ^ RAPID_SECRET[ 0 ] ^ m_size, m_b ^ RAPID_SECRET[ 1 ] );
	}
}
